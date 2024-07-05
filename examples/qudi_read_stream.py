import datetime
import rpyc

def on_init(input):
    host = '127.0.0.1'
    port = 65200
    module_name = 'instream'

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
    result.MaxNextExecutionDelay = datetime.timedelta(seconds=0.1)

    data = on_init.stream.read_data()
    for v, t in zip(*data):
        input.OutputStreams[0].Samples.append(DataStreamInstrument.BasicSample(v, t))

    return result

def on_exit(input):
    on_init.stream.stop_stream()
    on_init.connection.close()