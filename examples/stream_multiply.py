import datetime

def on_step(input):
    result = StreamManipulator.OutputData()
    result.MaxNextExecutionDelay = datetime.timedelta(seconds=2)
    result.MinNextExecutionDelay = datetime.timedelta(seconds=0.5)

    NumSamples = min(len(input.InputStreams[0].Samples), len(input.InputStreams[1].Samples))
    for i in range(0, NumSamples):
        input.OutputStreams[0].Samples.append(DataStreamInstrument.BasicSample(\
            input.InputStreams[0].Samples[i].Value * input.InputStreams[1].Samples[i].Value,\
            input.InputStreams[0].Samples[i].Time))
        
    result.LastConsumedSampleIDsPerInputStream.append(input.InputStreams[0].CalcLastConsumedSampleID(NumSamples))
    result.LastConsumedSampleIDsPerInputStream.append(input.InputStreams[1].CalcLastConsumedSampleID(NumSamples))
    
    return result