import time
import datetime
import random

def on_step(input):
    result = StreamManipulator.OutputData()
    result.MaxNextExecutionDelay = datetime.timedelta(seconds=.1)

    if round(time.time()) / 3 % 5 < 2:
        value = random.uniform(0.0, 1.0)
    else:
        value = random.uniform(2.0, 3.0)

    input.OutputStreams[0].Samples.append(DataStreamInstrument.BasicSample(value))
    
    return result