// This file is part of DynExp.

#include "stdafx.h"
#include "HardwareAdaptergRPC.h"

namespace DynExpHardware
{
	gRPCException::gRPCException(const grpc::Status Result, const std::source_location Location) noexcept
		: Exception((Result.error_code() == grpc::StatusCode::UNIMPLEMENTED && Result.error_message().empty()) ? UNIMPLEMENTED_ErrorMsg : Result.error_message(),
			Util::ErrorType::Error,
			Util::NumToT<int>(static_cast<std::underlying_type_t<grpc::StatusCode>>(Result.error_code())),
			Location)
	{
	}
}