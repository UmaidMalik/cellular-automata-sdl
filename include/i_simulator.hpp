#pragma once

class ISimulator {
public:
    virtual void Initialize() = 0;
    virtual void Step() = 0;
    virtual ~ISimulator() = default;
};