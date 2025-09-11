#pragma once

class IRenderer {
public:
    virtual void Initialize() = 0;
    virtual void Draw() = 0;
    virtual ~IRenderer() = default;
}; 