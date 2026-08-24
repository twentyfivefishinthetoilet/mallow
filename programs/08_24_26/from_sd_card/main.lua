counter = 0
image_index=0
init = true
function update()
    counter = counter + delta_time
end

function render()
    if init then
        fill_screen(0x0000)
        display_fb()
        init = false
    end
    if counter >= 5.0 then
        --[[
        if flip_flop then
            draw_image("test.bin", (320/2)-(150/2), (240/2)-(150/2))
        else
            draw_rect_filled((320/2)-25, (240/2)-25, 50, 50, 0xf81f)
        end
        ]]--
        local path = string.format("%d.bin", image_index)
        draw_image(path, (320/2)-(220/2), (240/2)-(220/2))

        if image_index > 15 then
            image_index = 0
        else
            image_index = image_index + 1
        end
        counter = 0
    end
end
