/**
 * @file   Keyboard.cpp
 * @author Vijaykumar Dangi
 */

#include <Windows.h>
#include "Keyboard.h"

namespace KeyboardInput
{
    // Variable Declaration
    unsigned char g_key_state[VK_OEM_CLEAR + 1];

    // Function Definition
    /**
     * @brief Update()
     */
    void Update()
    {
        // variable declaration
        BYTE _key_states[256];
        const BYTE HIGH_BIT = 1 << 7;
        
        // code
        ZeroMemory( _key_states, sizeof(_key_states));

        GetKeyboardState( _key_states);

        for( int i = VK_LBUTTON; i <= VK_OEM_CLEAR; ++i)
        {
            bool isDown = ( _key_states[i] & HIGH_BIT) != 0;
            bool wasDown = ( g_key_state[i] & EKeyState::IE_HELD) != 0;
            BYTE new_state = 0;

            if( isDown)
            {
                new_state |= EKeyState::IE_HELD;
            }

            if( isDown && !wasDown)
            {
                new_state |= EKeyState::IE_PRESSED;
            }

            if( !isDown && wasDown)
            {
                new_state |= EKeyState::IE_RELEASED;
            }

            g_key_state[i] = new_state;
        }
    }
    
    /**
     * @brief Update()
     */
    bool IsKeyPressed( unsigned char virtual_key)
    {
        // code
        return ((g_key_state[virtual_key] & EKeyState::IE_PRESSED) != 0);
    }

    /**
     * @brief Update()
     */
    bool IsKeyHeld( unsigned char virtual_key)
    {
        // code
        return ((g_key_state[virtual_key] & EKeyState::IE_HELD) != 0);
    }

    /**
     * @brief Update()
     */
    bool IsKeyReleased( unsigned char virtual_key)
    {
        // code
        return ((g_key_state[virtual_key] & EKeyState::IE_RELEASED) != 0);
    }

    /**
     * @brief Update()
     */
    unsigned char GetKeyState( unsigned char virtual_key)
    {
        // code
        return (g_key_state[virtual_key]);
    }
};

