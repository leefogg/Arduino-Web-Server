#pragma once
namespace HTTPStatusCode {
	
	// Information
	namespace Information {
		const byte
			Continue = 100,
			SwitchingProtocols = 101;
	}
	namespace Success {
		unsigned int const
			OK = 200,
			Created = 201,
			Accepted = 202,
			NonAuthoritativeInformation = 203,
			NoContent = 204,
			ResetContent = 205,
			PartialContent = 206;
	}
	namespace Redirection {
		unsigned int const
			MultipleChoices = 300,
			MovedPermanently = 301,
			Found = 302,
			SeeOther = 303,
			NotModified = 304,
			UseProxy = 305,
			//306 Removed in latest HTTP version
			TemporaryRedirect = 307;
	}
	namespace ClientError {
		unsigned int const
			BadRequest = 400,
			Unauthorized = 401,
			PaymentRequired = 402,
			Forbidden = 403,
			NotFound = 404,
			MethodNotAllowed = 405,
			NotAcceptable = 406,
			ProxyAuthenticationRequired = 407,
			RequestTimeout = 408,
			Conflict = 409,
			Gone = 410,
			LengthRequired = 411,
			PreconditionFailed = 412,
			RequestEntityTooLarge = 413,
			RequestURLTooLong = 414,
			UnsupportedMediaType = 415,
			RequestedRangeNotSatisfiable = 416,
			ExpectationFailed = 417;
	}
	namespace ServerError {
		unsigned int const
			InternalServerError = 500,
			NotImplemented = 501,
			BadGateway = 502,
			ServiceUnavailable = 503,
			GatewayTimeout = 504,
			HTTPVersionNotSupported = 505;
	}
}