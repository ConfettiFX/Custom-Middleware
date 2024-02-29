--[[ Set camera at space level --]]

-- local keyFrame0 = { position = { 133500, 44000, 0 }, lookAt = { 134000, 44000, 0 } }

local keyFrame0 = { position = { 0, 6000, 0 }, lookAt = { 0, 6000, 100 } }

loader.SetCameraPosition(keyFrame0.position[1], keyFrame0.position[2], keyFrame0.position[3])
loader.SetCameraLookAt(keyFrame0.lookAt[1], keyFrame0.lookAt[2], keyFrame0.lookAt[3])
loader.ResetFirstFrame()