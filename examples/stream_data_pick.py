def on_start(input):
    result = StreamManipulator.OutputData()

    input.OutputStreams[0].Clear()

    return result

def on_finished(input):
    result = StreamManipulator.OutputData()

    if len(input.InputStreams) > 1:
        with open(input.SaveFilename, "w") as file:
            file.write("Time;Value\n")
            for i in range(len(input.InputStreams[1].Samples)):
                file.write(f"{input.InputStreams[1].Samples[i].Time};{input.InputStreams[1].Samples[i].Value}\n")

        result.LastConsumedSampleIDsPerInputStream.append(input.InputStreams[0].ConsumeNone())
        result.LastConsumedSampleIDsPerInputStream.append(input.InputStreams[1].ConsumeAll())

    return result

def on_trigger(input):
    result = StreamManipulator.OutputData()

    if len(input.InputStreams[0].Samples) > 0:
        input.OutputStreams[0].Samples.append(DataStreamInstrument.BasicSample(\
            input.InputStreams[0].Samples[-1].Value, input.InputStreams[0].Samples[-1].Time))

    if len(input.InputStreams) > 1:
        result.LastConsumedSampleIDsPerInputStream.append(input.InputStreams[0].ConsumeAll())
        result.LastConsumedSampleIDsPerInputStream.append(input.InputStreams[1].ConsumeNone())
    
    return result