# This file is part of DynExp.

list(APPEND PROTO_FILES "proto/Common.proto")
list(APPEND PROTO_FILES "proto/NetworkAnalogIn.proto")
list(APPEND PROTO_FILES "proto/NetworkAnalogOut.proto")
list(APPEND PROTO_FILES "proto/NetworkDataStreamInstrument.proto")
list(APPEND PROTO_FILES "proto/NetworkDigitalIn.proto")
list(APPEND PROTO_FILES "proto/NetworkDigitalOut.proto")
list(APPEND PROTO_FILES "proto/NetworkSpectrometer.proto")
list(APPEND PROTO_FILES "proto/NetworkTimeTagger.proto")
list(APPEND PROTO_FILES "proto/WidefieldLocalization.proto")

set(PROTO_FILES ${PROTO_FILES} PARENT_SCOPE)