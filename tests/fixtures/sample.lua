-- Line comment
--[[ Block comment ]]
local x = 42 -- inline

local s = "hello -- not a comment"
local s2 = 'world -- not a comment'
local s3 = [[ long string -- not a comment ]]
local s4 = [=[ long string ]=] not end -- still string ]=]

--[=[
Multi-line
long comment
]=]
print(x)
