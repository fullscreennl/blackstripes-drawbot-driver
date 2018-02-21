penUp()

moveTo(0,1000.0)
penDown()

penup = false
local i = 0

function sleep(n)
  os.execute("sleep " .. tonumber(n))
end


while true do
    sleep(5)
    if penup == false then
        moveTo(100.0,1000.0)
        penUp()
        penup = true
    else
        moveTo(0.0,1000.0)
        penDown()
        penup = false
    end
end

