penUp()
local nib_size_mm = 3.0
local radius = 489.0
local num_cycles = (radius+1.0)/nib_size_mm
local numiterations = 3600 * num_cycles
local centerx = 500.0
local centery = 500.0
local i = 0
local degree_to_radian_fact = 0.0174532925

moveTo(989.0,500.0)
penDown()

while numiterations > i do
    local x = math.cos((i*degree_to_radian_fact)/10.0) * radius + centerx
    local y = math.sin((i*degree_to_radian_fact)/10.0) * radius + centery
    moveTo(x,y)
    radius = radius - nib_size_mm/3600.0
    i = i + 1
end

penUp()