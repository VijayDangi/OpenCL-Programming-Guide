/**
 * @file   Keyboard.h
 * @author Vijaykumar Dangi
 */

#pragma once

namespace KeyboardInput
{
    // Enum Declaration
    enum EKeyState
    {
        IE_NONE = 0,
        IE_PRESSED = 1 << 0,
        IE_HELD = 1 << 1,
        IE_RELEASED = 1 << 2
    };

    // Function Declaration
    void Update();
    bool IsKeyPressed( unsigned char virtual_key);
    bool IsKeyHeld( unsigned char virtual_key);
    bool IsKeyReleased( unsigned char virtual_key);

    unsigned char GetKeyState( unsigned char virtual_key);
};
