
#pragma once

namespace Policy {
constexpr short Success = 0;
constexpr short Unreachable = 1;
constexpr short ServerError = 2;
constexpr short Internal = 3;
constexpr short InvalidLogin = 4;
constexpr short NotValidated = 5;
constexpr short Cancelled = 6;
constexpr short NoEntitlement = 7;
constexpr short InvalidCode = 8;

constexpr int NoSocket = 2;
constexpr int NotResolved = 4;
constexpr int NotConnected = 5;
constexpr int TimedOut = 6;
constexpr int Broken = 7;
constexpr int Stopped = 8;
constexpr int HttpError = 9;
constexpr int HttpUnauthorized = 10;
constexpr int NotHttp = 13;

constexpr unsigned kDelays[] = {1000, 2000, 4000, 8000};
constexpr unsigned kRetries = sizeof(kDelays) / sizeof(kDelays[0]);
constexpr unsigned kRetriesNotResolved = 1;
constexpr unsigned kBudget = 25000;

constexpr bool Temporary(short result) {
    return result == Unreachable || result == ServerError || result == Internal;
}

constexpr unsigned Retries(short result, int transport, int status) {
    if (!Temporary(result) || transport == NotHttp || (transport == HttpError && status < 500))
        return 0;
    return transport == NotResolved ? kRetriesNotResolved : kRetries;
}

constexpr unsigned Delay(short result, int transport, int status, unsigned retried, unsigned elapsed) {
    if (retried >= Retries(result, transport, status))
        return 0;
    unsigned delay = kDelays[retried];
    return elapsed <= kBudget && delay <= kBudget - elapsed ? delay : 0;
}
}

