// Copyright (c) 2015-2015 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <chrono>
#include <complex>
#include <iostream>
#include <algorithm> //min/max
#include <cstring> //memcpy

/***********************************************************************
 * |PothosDoc Wave Trigger
 *
 * The Wave Trigger monitors an input port for configurable trigger events.
 * When an event is detected, the trigger block forwards selected samples
 * from all input ports to the output port as a Packet object with metadata.
 *
 * The Wave Trigger is the processing back-end for the graphical Waveform Monitor,
 * however its inputs, outputs, and configuration is well documented,
 * so it can be used in a variety of other situations if needed.
 *
 * |category /Utility
 *
 * |param numPorts[Num Ports] The number of input ports.
 * |default 1
 * |widget SpinBox(minimum=1)
 * |preview disable
 *
 * |param dataPoints[Data Points] The number of elements to yield on each channel when triggered.
 * |default 1024
 * |widget SpinBox(minimum=0)
 *
 * |param alignment[Alignment] Synchronous or asynchronous multi-channel consumption pattern.
 * When in synchronous mode, work() consumes the same amount from all channels to preserve alignment.
 * When in asynchronous mode, work() consumes all available input from each channel independently.
 * |default false
 * |option [Disable] false
 * |option [Enable] true
 *
 * |param holdOff[Hold Off] Hold off on subsequent trigger events for this many samples.
 * After a trigger event occurs, <em>hold off</em> disables trigger sweeping until
 * the specified number of samples has been consumed.
 * |units samples
 * |default 1024
 *
 * |param channel[Channel] Which input channel to monitor for trigger events.
 * |default 0
 * |widget SpinBox(minimum=0)
 *
 * |param sweepRate[Sweep Rate] The rate of the trigger sweep.
 * In automatic mode, this rate sets the timer that forces a trigger event.
 * Or in the case of inadequate input after a trigger event,
 * this rate acts as a timeout to flush the available samples.
 * |units events/sec
 * |default 1.0
 *
 * |param slope[Slope] The required slope of the trigger detection.
 * <ul>
 * <li>Positive slope means that the trigger will be activated when the level is rises above the specified trigger level.</li>
 * <li>Negative slope means that the trigger will be activated when the level is falls below the specified trigger level.</li>
 * <li>Level means that the trigger will be activated when the trigger level is detected, regardless of the slope.</li>
 * </ul>
 * |default "POS"
 * |option [Positive] "POS"
 * |option [Negative] "NEG"
 * |option [Level] "LEVEL"
 *
 * |param mode [Mode] The operational mode of the triggering system.
 * <ul>
 * <li>In automatic mode, the trigger event is forced by timer if none occurs.</li>
 * <li>In normal mode, samples are only forwarded when a trigger event occurs.</li>
 * <li>In disabled mode, trigger sweeping is disabled and samples are not forwarded.</li>
 * <li>In one-shot mode, the trigger event occurs once and then becomes disabled.</li>
 * </ul>
 * |default "AUTOMATIC"
 * |option [Automatic] "AUTOMATIC"
 * |option [Normal] "NORMAL"
 * |option [Disabled] "DISABLED"
 * |option [One-Shot] "ONESHOT"
 *
 * |param level [Level] The value of the input required for a trigger event.
 * |default 0.5
 * |widget DoubleSpinBox()
 *
 * |param position [Position] The offset in samples before the trigger event.
 * When the samples are forwarded to the output,
 * the trigger event occurs <em>position</em> number of samples into the array.
 * |units samples
 * |default 128
 * |widget SpinBox(minimum=0)
 *
 * |factory /blocks/wave_trigger()
 * |initializer setNumPorts(numPorts)
 * |setter setDataPoints(dataPoints)
 * |setter setAlignment(alignment)
 * |setter setHoldOff(holdOff)
 * |setter setChannel(channel)
 * |setter setTimeout(timeout)
 * |setter setSlope(slope)
 * |setter setMode(mode)
 * |setter setLevel(level)
 * |setter setPosition(position)
 **********************************************************************/
class WaveTrigger : public Pothos::Block
{
public:
    static Block *make(void)
    {
        return new WaveTrigger();
    }

    WaveTrigger(void):
        _dataPoints(0),
        _alignment(true),
        _holdOff(0),
        _channel(0),
        _sweepRate(1.0),
        _posSlope(false),
        _negSlope(false),
        _automaticMode(false),
        _normalMode(false),
        _oneShotMode(false),
        _level(0.0),
        _position(0)
    {
        this->setupInput(0);
        this->setupOutput(0);

        this->registerCall(this, POTHOS_FCN_TUPLE(WaveTrigger, setNumPorts));
        this->registerCall(this, POTHOS_FCN_TUPLE(WaveTrigger, setDataPoints));
        this->registerCall(this, POTHOS_FCN_TUPLE(WaveTrigger, getDataPoints));
        this->registerCall(this, POTHOS_FCN_TUPLE(WaveTrigger, setAlignment));
        this->registerCall(this, POTHOS_FCN_TUPLE(WaveTrigger, getAlignment));
        this->registerCall(this, POTHOS_FCN_TUPLE(WaveTrigger, setHoldOff));
        this->registerCall(this, POTHOS_FCN_TUPLE(WaveTrigger, getHoldOff));
        this->registerCall(this, POTHOS_FCN_TUPLE(WaveTrigger, setChannel));
        this->registerCall(this, POTHOS_FCN_TUPLE(WaveTrigger, getChannel));
        this->registerCall(this, POTHOS_FCN_TUPLE(WaveTrigger, setSweepRate));
        this->registerCall(this, POTHOS_FCN_TUPLE(WaveTrigger, getSweepRate));
        this->registerCall(this, POTHOS_FCN_TUPLE(WaveTrigger, setSlope));
        this->registerCall(this, POTHOS_FCN_TUPLE(WaveTrigger, getSlope));
        this->registerCall(this, POTHOS_FCN_TUPLE(WaveTrigger, setMode));
        this->registerCall(this, POTHOS_FCN_TUPLE(WaveTrigger, getMode));
        this->registerCall(this, POTHOS_FCN_TUPLE(WaveTrigger, setLevel));
        this->registerCall(this, POTHOS_FCN_TUPLE(WaveTrigger, getLevel));
        this->registerCall(this, POTHOS_FCN_TUPLE(WaveTrigger, setPosition));
        this->registerCall(this, POTHOS_FCN_TUPLE(WaveTrigger, getPosition));

        //initialization
        this->setDataPoints(1024);
        this->setAlignment(true);
        this->setHoldOff(1024);
        this->setChannel(0);
        this->setSweepRate(1.0);
        this->setSlope("POS");
        this->setMode("AUTOMATIC");
        this->setLevel(0.5);
        this->setPosition(128);
    }

    void setNumPorts(const size_t numPorts)
    {
        for (size_t i = this->inputs().size(); i < numPorts; i++) this->setupInput(i);
    }

    void setDataPoints(const size_t numElems)
    {
        _dataPoints = numElems;
    }

    size_t getDataPoints(void) const
    {
        return _dataPoints;
    }

    void setAlignment(const bool enabled)
    {
        _alignment = enabled;
    }

    bool getAlignment(void) const
    {
        return _alignment;
    }

    void setHoldOff(const size_t holdOff)
    {
        _holdOff = holdOff;
    }

    bool getHoldOff(void) const
    {
        return _holdOff;
    }

    void setChannel(const size_t channel)
    {
        if (channel >= this->inputs().size()) throw Pothos::InvalidArgumentException("WaveTrigger::setChannel()", "channel out of range");
        _channel = channel;
    }

    bool getChannel(void) const
    {
        return _channel;
    }

    void setSweepRate(const double rate)
    {
        if (rate <= 0.0) throw Pothos::InvalidArgumentException("WaveTrigger::setSweepRate()", "sweep rate must be positive");
        _sweepRate = rate;
    }

    double getSweepRate(void) const
    {
        return _sweepRate;
    }

    void setSlope(const std::string &slope)
    {
        if (slope == "POS")
        {
            _posSlope = true;
            _negSlope = false;
        }
        else if (slope == "NEG")
        {
            _posSlope = false;
            _negSlope = true;
        }
        else if (slope == "LEVEL")
        {
            _posSlope = true;
            _negSlope = true;
        }
        else
        {
            throw Pothos::InvalidArgumentException("WaveTrigger::setSlope("+slope+")", "unknown slope setting");
        }
        _slopeStr = slope;
    }

    std::string getSlope(void) const
    {
        return _slopeStr;
    }

    void setMode(const std::string &mode)
    {
        if (mode == "AUTOMATIC")
        {
            _automaticMode = true;
            _normalMode = false;
            _oneShotMode = false;
        }
        else if (mode == "NORMAL")
        {
            _automaticMode = false;
            _normalMode = true;
            _oneShotMode = false;
        }
        else if (mode == "DISABLED")
        {
            _automaticMode = false;
            _normalMode = false;
            _oneShotMode = false;
        }
        else if (mode == "ONESHOT")
        {
            _automaticMode = false;
            _normalMode = false;
            _oneShotMode = true;
        }
        else
        {
            throw Pothos::InvalidArgumentException("WaveTrigger::setMode("+mode+")", "unknown mode setting");
        }
        _modeStr = mode;
    }

    std::string getMode(void) const
    {
        return _modeStr;
    }

    void setLevel(const double level)
    {
        _level = level;
    }

    double getLevel(void) const
    {
        return _level;
    }

    void setPosition(const size_t position)
    {
        _position = position;
    }

    size_t getPosition(void) const
    {
        return _position;
    }

    void activate(void)
    {
    }

    void deactivate(void)
    {
    }

    void work(void)
    {
    }

    void propagateLabels(const Pothos::InputPort *)
    {
        //do not propagate labels
    }

private:

    //configuration settings
    size_t _dataPoints;
    bool _alignment;
    size_t _holdOff;
    size_t _channel;
    double _sweepRate;
    std::string _slopeStr;
    bool _posSlope;
    bool _negSlope;
    std::string _modeStr;
    bool _automaticMode;
    bool _normalMode;
    bool _oneShotMode;
    double _level;
    size_t _position;

    //state tracking

    std::vector<std::chrono::high_resolution_clock::time_point> _lastTriggerTimes;
    std::vector<Pothos::Packet> _accumulationBuffs;
};

static Pothos::BlockRegistry registerWaveTrigger(
    "/blocks/wave_trigger", &WaveTrigger::make);
