import datetime
from simple_pid import PID

def on_init(input):
    on_init.time_diff = 0.1
    on_init.pid = PID(5, 0.1, 0.2, setpoint=20.0)

def on_step(input):
    result = StreamManipulator.OutputData()
    result.MaxNextExecutionDelay = datetime.timedelta(seconds=1)
    result.MinNextExecutionDelay = datetime.timedelta(seconds=on_init.time_diff)

    if len(input.InputStreams[1].Samples):
        on_init.pid.setpoint = input.InputStreams[1].Samples[0].Value
        print("PID controller setpoint changed to {}.".format(on_init.pid.setpoint))
    
    if len(input.InputStreams[0].Samples):
        temperature = input.InputStreams[0].Samples[-1].Value
        heating_power = on_init.pid(temperature)

        input.OutputStreams[0].Samples.append(DataStreamInstrument.BasicSample(heating_power))
    
    return result