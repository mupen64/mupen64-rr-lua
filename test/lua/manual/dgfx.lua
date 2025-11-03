emu.atdrawd2d(function()
    dgfx.add(DGfxCommandType.RECT, {
        {
            color = 0xFFFF0000,
            thickness = 1,
            x = 50,
            y = 50,
            w = 100,
            h = 100,
        },
        {
            color = 0xFF00FFFF,
            thickness = 1,
            x = 200,
            y = 50,
            w = 100,
            h = 100,
        }
    })
    dgfx.add(DGfxCommandType.FILLED_RECT, {
        {
            color = 0xFFFF0000,
            x = 50,
            y = 150,
            w = 100,
            h = 100,
        },
        {
            color = 0xFF00FFFF,
            x = 200,
            y = 150,
            w = 100,
            h = 100,
        }
    })
end)
