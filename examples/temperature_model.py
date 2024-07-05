import datetime

def on_init(input):
    on_init.time_diff = 0.1
    on_init.thermal_mass = 10.0
    on_init.thermal_resistance = 5.0
    on_init.current_time = 0.0
    on_init.current_heating_power = 0.0
    on_init.latency = 10.0
    on_init.intermediate_temp = 20.0
    on_init.current_temp = on_init.intermediate_temp

def on_step(input):
    result = StreamManipulator.OutputData()
    result.MaxNextExecutionDelay = datetime.timedelta(seconds=on_init.time_diff)
    result.MinNextExecutionDelay = datetime.timedelta(seconds=on_init.time_diff)
    
    if len(input.InputStreams[0].Samples):
        on_init.current_heating_power = input.InputStreams[0].Samples[0].Value

    on_init.current_time = on_init.current_time + on_init.time_diff
    on_init.intermediate_temp = on_init.intermediate_temp + \
        on_init.current_heating_power * on_init.time_diff / (on_init.thermal_mass * on_init.thermal_resistance)
    on_init.current_temp = on_init.current_temp + \
        (on_init.intermediate_temp - on_init.current_temp) * on_init.time_diff / on_init.latency
    
    input.OutputStreams[0].Samples.append(DataStreamInstrument.BasicSample( \
        on_init.current_temp, on_init.current_time))
    
    return result