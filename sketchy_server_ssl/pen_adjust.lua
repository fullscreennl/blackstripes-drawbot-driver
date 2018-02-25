penUp()
moveTo(0,1000.0)
penDown()
penup = false
local i = 0

function sleep(n)
  os.execute("sleep " .. tonumber(n))
end

while true do
    if penup == false then
        moveTo(100.0,1000.0)
        penUp()
        penup = true
        c = 10
        while c > 0 do
            print("pen up!")
            sleep(1)
            c = c - 1
        end
    else
        moveTo(0.0,1000.0)
        penDown()
        penup = false
        c = 10
        while c > 0 do
            print("pen down!")
            sleep(1)
            c = c - 1
        end
    end
end

