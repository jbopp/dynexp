import datetime
import rpyc

def on_init(input):
    host = '141.20.45.252'
    port = 12345
    module_name = 'wavemeter'

    protocol_config = {
        'allow_all_attrs': True,
        'allow_setattr': True,
        'allow_delattr': True,
        'allow_pickle': True,
        'sync_request_timeout': 3600
    }

    on_init.connection = rpyc.connect(host=host, port=port, config=protocol_config)
    on_init.stream = on_init.connection.root.get_module_instance(module_name)

    on_init.value_unit = on_init.stream.constraints.channel_units
    on_init.stream.start_stream()

def on_step(input):
    result = StreamManipulator.OutputData()
    result.MaxNextExecutionDelay = datetime.timedelta(seconds=6)
    result.MinNextExecutionDelay = datetime.timedelta(seconds=5)

    data = on_init.stream.read_data()
    frequencies = data[0].reshape(-1, 4)
    times = data[1]

    for v, t in zip(frequencies, times):
        input.OutputStreams[0].Samples.append(DataStreamInstrument.BasicSample(v[3], t))

    return result

def on_exit():
    on_init.stream.stop_stream()
    on_init.connection.close()