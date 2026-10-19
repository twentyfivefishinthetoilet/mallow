counter = 0
flip_flop = true;
function update()
    counter = counter + delta_time
end

function render()
    if counter >= 60.0 then
        if flip_flop then
            draw_rect_filled(25, 25, 50, 50, 0x07e0)
        else
            draw_rect_filled(25, 25, 50, 50, 0xf81f)
        end
        draw_rect(50, 75, 100, 100, 0xf800)
        display_fb()

        counter = 0
        flip_flop = not flip_flop
    end
end
