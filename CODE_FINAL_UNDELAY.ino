#include <Feelix.h>
#include <math.h>

Feelix feelix = Feelix();


/* =========================================================
   MODE
   ========================================================= */

enum Mode
{
  PRESS_MODE,
  KICK_MODE,
  OSCILLATION_MODE,
  FINAL_SETTLE_MODE
};

Mode mode = PRESS_MODE;


/* =========================================================
   CENTER
   ========================================================= */

float centerAngle = 0.0;


/* =========================================================
   SOUND CLASSIFICATION
   ========================================================= */

/*
   小于 0.4 rad：
   当作轻微误触，不播放声音
*/
const float SOUND_MIN_THRESHOLD = 0.40;


/*
   轻推 / 重推分界
*/
const float HEAVY_PUSH_THRESHOLD = 2.00;


/* =========================================================
   PRESS PARAMETERS
   ========================================================= */

const float PRESS_THRESHOLD = 0.02;

const float RESIST_FULL_DEPTH = 3.0;

const float MIN_RETURN_TORQUE = 0.90;

const float MAX_RESIST_TORQUE = 3.0;

const float RESIST_EXPONENT = 1.55;

const float TORQUE_SIGN = +1.0;


/* =========================================================
   PRESS STATE
   ========================================================= */

bool pressed = false;

float peakDepth = 0.0;

unsigned long lastPeakTime = 0;


/* =========================================================
   RELEASE DETECTION
   ========================================================= */

const float RELEASE_DROP = 0.012;


/*
   原来 180ms
   现在降到 80ms

   这样松手之后声音更快触发。
*/
const unsigned long RELEASE_TIMEOUT = 80;


const float PEAK_UPDATE_EPSILON = 0.008;


/* =========================================================
   KICK
   ========================================================= */

const float KICK_TORQUE = 3.4;

const unsigned long KICK_TIME = 110;

unsigned long kickStartTime = 0;


/* =========================================================
   SPRING / OSCILLATION
   ========================================================= */

const float OSCILLATION_PERIOD = 0.65;

const float BASE_DAMPING = 3.8;

const float MIN_DAMPING = 0.38;

const float DAMPING_DEPTH_GAIN = 0.75;

const float STOP_AMPLITUDE = 0.025;


/* =========================================================
   SWING AMPLITUDE
   ========================================================= */

const float MAX_SWING_AMPLITUDE = 1.45;

const float SWING_GAIN = 0.90;


/* =========================================================
   OSCILLATION DURATION
   ========================================================= */

float minimumOscillationTime = 0.0;

const float MIN_OSC_TIME = 0.25;

const float OSC_TIME_DEPTH_GAIN = 0.34;

const float MAX_MIN_OSC_TIME = 3.5;


/* =========================================================
   OSCILLATION STATE
   ========================================================= */

float swingAmplitude = 0.0;

float activeDamping = BASE_DAMPING;

float oscillationPhase = 0.0;

unsigned long oscillationStartTime = 0;


/* =========================================================
   FINAL SETTLE
   ========================================================= */

float settleStartAngle = 0.0;

unsigned long settleStartTime = 0;

const unsigned long FINAL_SETTLE_TIME = 280;


/* =========================================================
   HELPERS
   ========================================================= */

float clampFloat(
  float value,
  float minimum,
  float maximum
)
{
  if (value < minimum)
  {
    return minimum;
  }

  if (value > maximum)
  {
    return maximum;
  }

  return value;
}


float smoothStep(float x)
{
  x = clampFloat(
    x,
    0.0,
    1.0
  );

  return x * x * (3.0 - 2.0 * x);
}


/* =========================================================
   SOUND EVENT
   ========================================================= */

void sendSoundEvent()
{
  /*
     这里必须用 println()

     因为 Python 用 readline()
     一看到换行就会立刻收到消息。
  */

  if (peakDepth < SOUND_MIN_THRESHOLD)
  {
    Serial.println("SOUND_NONE");
  }

  else if (peakDepth < HEAVY_PUSH_THRESHOLD)
  {
    Serial.println("SOUND_LIGHT");
  }

  else
  {
    Serial.println("SOUND_HEAVY");
  }
}


/* =========================================================
   CALCULATE SPRING RESPONSE
   ========================================================= */

void calculateSpringResponse()
{
  /* -------------------------------------------------------
     Amplitude
     ------------------------------------------------------- */

  swingAmplitude =
    peakDepth *
    SWING_GAIN;


  swingAmplitude =
    clampFloat(
      swingAmplitude,
      0.05,
      MAX_SWING_AMPLITUDE
    );


  /* -------------------------------------------------------
     Damping
     ------------------------------------------------------- */

  activeDamping =
    BASE_DAMPING /
    (
      1.0 +
      DAMPING_DEPTH_GAIN *
      peakDepth
    );


  if (
    activeDamping <
    MIN_DAMPING
  )
  {
    activeDamping =
      MIN_DAMPING;
  }


  /* -------------------------------------------------------
     Minimum oscillation time
     ------------------------------------------------------- */

  minimumOscillationTime =
    MIN_OSC_TIME +
    peakDepth *
    OSC_TIME_DEPTH_GAIN;


  minimumOscillationTime =
    clampFloat(
      minimumOscillationTime,
      MIN_OSC_TIME,
      MAX_MIN_OSC_TIME
    );
}


/* =========================================================
   START RELEASE
   ========================================================= */

void startRelease()
{
  /*
     最重要：
     一判断出轻/重推，立刻发给 Python
  */

  sendSoundEvent();


  /*
     然后 motor 自己继续做回弹
  */

  calculateSpringResponse();


  kickStartTime =
    millis();


  mode =
    KICK_MODE;
}


/* =========================================================
   START OSCILLATION
   ========================================================= */

void startOscillationFromCurrentState()
{
  float currentOffset =
    feelix.angle -
    centerAngle;


  float normalizedPosition =
    currentOffset /
    swingAmplitude;


  normalizedPosition =
    clampFloat(
      normalizedPosition,
      -1.0,
      1.0
    );


  float basePhase =
    acosf(
      normalizedPosition
    );


  if (
    feelix.velocity >=
    0.0
  )
  {
    oscillationPhase =
      -basePhase;
  }

  else
  {
    oscillationPhase =
      basePhase;
  }


  oscillationStartTime =
    millis();


  mode =
    OSCILLATION_MODE;
}


/* =========================================================
   RESET
   ========================================================= */

void resetInteraction()
{
  pressed =
    false;


  peakDepth =
    0.0;


  swingAmplitude =
    0.0;


  activeDamping =
    BASE_DAMPING;


  oscillationPhase =
    0.0;


  minimumOscillationTime =
    0.0;


  lastPeakTime =
    millis();
}


/* =========================================================
   SETUP
   ========================================================= */

void setup()
{
  Serial.begin(115200);


  feelix.init();


  feelix.sensor_dir =
    Direction::CCW;


  for (
    int i = 0;
    i < 500;
    i++
  )
  {
    feelix.run();

    delay(2);
  }


  centerAngle =
    feelix.angle;


  lastPeakTime =
    millis();


  Serial.println(
    "SYSTEM_READY"
  );
}


/* =========================================================
   LOOP
   ========================================================= */

void loop()
{
  feelix.run();


  float error =
    feelix.angle -
    centerAngle;



  /* =======================================================
     MODE 0
     PRESS
     ======================================================= */

  if (
    mode ==
    PRESS_MODE
  )
  {

    float depth =
      0.0;


    if (
      error <
      0.0
    )
    {
      depth =
        -error;
    }



    /* -----------------------------------------------------
       PRESS RESISTANCE
       ----------------------------------------------------- */

    float torqueCommand =
      0.0;


    if (
      depth >
      0.0
    )
    {

      float normalized =
        depth /
        RESIST_FULL_DEPTH;


      normalized =
        clampFloat(
          normalized,
          0.0,
          1.0
        );


      float resistance =
        MIN_RETURN_TORQUE
        +
        (
          MAX_RESIST_TORQUE
          -
          MIN_RETURN_TORQUE
        )
        *
        powf(
          normalized,
          RESIST_EXPONENT
        );


      torqueCommand =
        TORQUE_SIGN *
        resistance;
    }


    feelix.move_feelix(
      torqueCommand,
      MotionControlType::torque
    );



    /* -----------------------------------------------------
       RECORD PEAK
       ----------------------------------------------------- */

    if (
      depth >
      PRESS_THRESHOLD
    )
    {

      pressed =
        true;


      if (
        depth
        >
        peakDepth
        +
        PEAK_UPDATE_EPSILON
      )
      {

        peakDepth =
          depth;


        lastPeakTime =
          millis();
      }
    }



    /* -----------------------------------------------------
       RELEASE DETECTION
       ----------------------------------------------------- */

    if (
      pressed
      &&
      peakDepth
      >
      PRESS_THRESHOLD
    )
    {

      bool movedBack =
        (
          peakDepth -
          depth
        )
        >
        RELEASE_DROP;


      bool stoppedAtPeak =
        (
          millis() -
          lastPeakTime
        )
        >
        RELEASE_TIMEOUT;


      if (
        movedBack
        ||
        stoppedAtPeak
      )
      {
        startRelease();
      }
    }
  }



  /* =======================================================
     MODE 1
     KICK
     ======================================================= */

  else if (
    mode ==
    KICK_MODE
  )
  {

    unsigned long elapsed =
      millis() -
      kickStartTime;


    feelix.move_feelix(
      TORQUE_SIGN *
      KICK_TORQUE,
      MotionControlType::torque
    );


    if (
      elapsed >=
      KICK_TIME
    )
    {
      startOscillationFromCurrentState();
    }
  }



  /* =======================================================
     MODE 2
     JELLY OSCILLATION
     ======================================================= */

  else if (
    mode ==
    OSCILLATION_MODE
  )
  {

    float t =
      (
        millis() -
        oscillationStartTime
      )
      /
      1000.0;


    float envelope =
      swingAmplitude
      *
      expf(
        -activeDamping *
        t
      );


    float omega =
      2.0 *
      PI /
      OSCILLATION_PERIOD;


    float target =
      centerAngle
      +
      envelope
      *
      cosf(
        omega *
        t
        +
        oscillationPhase
      );


    feelix.move_feelix(
      target,
      MotionControlType::angle
    );



    bool enoughTime =
      t >=
      minimumOscillationTime;


    bool smallEnough =
      envelope <
      STOP_AMPLITUDE;


    if (
      enoughTime
      &&
      smallEnough
    )
    {

      settleStartAngle =
        feelix.angle;


      settleStartTime =
        millis();


      mode =
        FINAL_SETTLE_MODE;
    }
  }



  /* =======================================================
     MODE 3
     FINAL SETTLE
     ======================================================= */

  else if (
    mode ==
    FINAL_SETTLE_MODE
  )
  {

    unsigned long elapsed =
      millis() -
      settleStartTime;


    float progress =
      (float)elapsed /
      (float)FINAL_SETTLE_TIME;


    progress =
      clampFloat(
        progress,
        0.0,
        1.0
      );


    float s =
      smoothStep(
        progress
      );


    float target =
      settleStartAngle
      +
      (
        centerAngle -
        settleStartAngle
      )
      *
      s;


    feelix.move_feelix(
      target,
      MotionControlType::angle
    );


    if (
      elapsed >=
      FINAL_SETTLE_TIME
    )
    {

      mode =
        PRESS_MODE;


      resetInteraction();
    }
  }
}