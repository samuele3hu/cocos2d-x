local GAME_SCENE = 
{
    SCENE_UI     = 1,
    SCENE_WORLD  = 2,
    SCENE_DIALOG = 3,
    SCENE_OSD    = 4,
    SCENE_COUNT  = 5,
}


local GAME_LAYER = 
{
    LAYER_BACKGROUND = 1,
    LAYER_DEFAULT    = 2,
    LAYER_MIDDLE     = 3,
    LAYER_TOP        = 4,
    LAYER_COUNT      = 5,
}

local GAME_CAMERAS_ORDER =
{
    CAMERA_WORLD_3D_SKYBOX = 1,
    CAMERA_WORLD_3D_SCENE  = 2,
    CAMERA_UI_2D           = 3,
    CAMERA_DIALOG_2D_BASE  = 4,
    CAMERA_DIALOG_3D_MODEL = 5,
    CAMERA_DIALOG_2D_ABOVE = 6,
    CAMERA_OSD_2D_BASE     = 7,
    CAMERA_OSD_3D_MODEL    = 8,
    CAMERA_OSD_2D_ABOVE    = 9,
    CAMERA_COUNT           = 10,
}
local s_CF  = 
{
    cc.CameraFlag.USER1,
    cc.CameraFlag.DEFAULT,
    cc.CameraFlag.USER2,
    cc.CameraFlag.USER3,
}

local s_CM =
{
    s_CF[1],
    s_CF[2],
    s_CF[3],
    s_CF[4],
}

local s_CameraNames =
{
    "World 3D Skybox",
    "World 3D Scene",
    "UI 2D",
    "Dialog 2D Base",
    "Dialog 3D Model",
    "Dialog 2D Above",
    "OSD 2D Base",
    "OSD 3D Model",
    "OSD 2D Above",
}

local s_scenePositons = 
{
    cc.vec3(0, 0, 0),
    cc.vec3(0, 10000, 0),
    cc.vec3(10000, 0, 0),
    cc.vec3(0, -10000, 0),
}

local SkinType =
{
    HAIR     = 1,
    GLASSES  = 2,
    FACE     = 3,
    UPPER_BODY = 4,
    HAND     = 5,
    PANTS    = 6,
    SHOES    = 7,
    MAX_TYPE = 8,
}

----------------------------------------
----Player
----------------------------------------

local PLAER_STATE =
{
    LEFT     = 0,
    RIGHT    = 1,
    IDLE     = 2,
    FORWARD  = 3, 
    BACKWARD = 4,
}

local PLAYER_HEIGHT = 0

local camera_offset = cc.vec3(0, 45, 60)

local Player = class("Player", function(file, cam, terrain)
    local sprite = cc.Sprite3D:create(file)
    if nil ~= sprite then
        sprite._headingAngle = 0
        sprite._playerState = PLAER_STATE.IDLE
        sprite._cam = cam
        sprite._terrain = terrain
    end
    return sprite
end)

function Player:ctor()
    -- body
    self:init()      
end

function Player:init()
    self._headingAxis = cc.vec3(0.0, 0.0, 0.0)
    self:scheduleUpdateWithPriorityLua(function(dt)
        local curPos = self:getPosition3D()
        if self._playerState == PLAER_STATE.IDLE then

        elseif self._playerState == PLAER_STATE.FORWARD then
            local newFaceDir = cc.vec3( self._targetPos.x - curPos.x, self._targetPos.y - curPos.y, self._targetPos.z - curPos.z)
            newFaceDir.y = 0.0
            newFaceDir = cc.vec3normalize(newFaceDir)
            local offset = cc.vec3(newFaceDir.x * 25.0 * dt, newFaceDir.y * 25.0 * dt, newFaceDir.z * 25.0 * dt)
            curPos = cc.vec3(curPos.x + offset.x, curPos.y + offset.y, curPos.z + offset.z)
            self:setPosition3D(curPos)
        elseif self._playerState ==  PLAER_STATE.BACKWARD then
            
            local transform   = self:getNodeToWorldTransform()
            local forward_vec = cc.vec3(-transform[9], -transform[10], -transform[11])
            forward_vec = cc.vec3normalize(forward_vec)
            self:setPosition3D(cc.vec3(curPos.x - forward_vec.x * 15 * dt, curPos.y - forward_vec.y * 15 * dt, curPos.z - forward_vec.z * 15 *dt))
        elseif self._playerState == PLAER_STATE.LEFT then
            player:setRotation3D(cc.vec3(curPos.x, curPos.y + 25 * dt, curPos.z))
        elseif self._playerState == PLAER_STATE.RIGHT then
            player:setRotation3D(cc.vec3(curPos.x, curPos.y - 25 * dt, curPos.z))
        end

        local normal = cc.vec3(0.0, 0.0, 0.0)
        local player_h, normal = self._terrain:getHeight(self:getPositionX(), self:getPositionZ(), normal)
        self:setPositionY(player_h + PLAYER_HEIGHT)

        --need to scriptfile
        local q2 = cc.quaternion_createFromAxisAngle(cc.vec3(0, 1, 0), -math.pi)
        local headingQ = cc.quaternion_createFromAxisAngle(self._headingAxis, self._headingAngle)
        local x = headingQ.w * q2.x + headingQ.x * q2.w + headingQ.y * q2.z - headingQ.z * q2.y
        local y = headingQ.w * q2.y - headingQ.x * q2.z + headingQ.y * q2.w + headingQ.z * q2.x
        local z = headingQ.w * q2.z + headingQ.x * q2.y - headingQ.y * q2.x + headingQ.z * q2.w
        local w = headingQ.w * q2.w - headingQ.x * q2.x - headingQ.y * q2.y - headingQ.z * q2.z
        headingQ = cc.quaternion(x, y, z, w)
        self:setRotationQuat(headingQ)

        local vec_offset = cc.vec4(camera_offset.x, camera_offset.y, camera_offset.z, 1)
        local transform  = self:getNodeToWorldTransform()
        local dst = cc.vec4(0.0, 0.0, 0.0, 0.0)
        vec_offset = mat4_transformVector(transform, vec_offset, dst)
        local playerPos = self:getPosition3D()
        self._cam:setPosition3D(cc.vec3(playerPos.x + camera_offset.x, playerPos.y + camera_offset.y, playerPos.z + camera_offset.z))
        self:updateState()
    end, 0)

    self:registerScriptHandler(function (event)
        -- body
        if "exit" == event then
            self:unscheduleUpdate()
        end
    end)
end

function Player:updateState()
    if self._playerState == PLAER_STATE.FORWARD then
        local player_pos = cc.p(self:getPositionX(),self:getPositionZ())
        local targetPos = cc.p(self._targetPos.x, self._targetPos.z)
        local dist = cc.pGetDistance(player_pos, targetPos)
        if dist < 1 then
            self._playerState = PLAER_STATE.IDLE
        end
    end
end



local Scene3DTestScene = class("Scene3DTestScene",function()
    local scene = cc.Scene:create()
    return scene
end)

function Scene3DTestScene:ctor()
    self._worldScene = nil
    self._dlgScene   = nil
    self._osdScene   = nil
    self._textureCube = nil
    self._skyBox     = nil
    self._terrain    = nil
    self._player     = nil
    self._playerItem = nil
    self._detailItem = nil
    self._descItem   = nil
    self._ui         = nil
    self._playerDlg  = nil
    self._detailDlg  = nil
    self._descDlg    = nil
    self._monsters   = {nil, nil}
    self._skins      = {}
    self._curSkin    = {}

    local function onNodeEvent(event)
        if "enter" == event then
            -- self:onEnter()
        elseif "exit" == event then
            self:onExit()
        end
    end
    self:registerScriptHandler(onNodeEvent)

    self:init()
end

function Scene3DTestScene:init()
    cc.Director:getInstance():setDisplayStats(false)

    self._gameCameras = {}
    for i = 1, GAME_CAMERAS_ORDER.CAMERA_COUNT do
        self._gameCameras[i] = nil
    end
    local visibleSize = cc.Director:getInstance():getVisibleSize()
    local ca = nil   -- temp variable


    --create world 3D scene, this scene has two camera
    self._worldScene = cc.Node:create()
    --[[
        disable default camera
        create a camera to look the skybox
    ]]--
    self._gameCameras[GAME_CAMERAS_ORDER.CAMERA_WORLD_3D_SKYBOX] = cc.Camera:createPerspective(60,
                                      visibleSize.width / visibleSize.height,
                                      10,
                                      1000)
    ca = self._gameCameras[GAME_CAMERAS_ORDER.CAMERA_WORLD_3D_SKYBOX]
           
    ca:setDepth(GAME_CAMERAS_ORDER.CAMERA_WORLD_3D_SKYBOX)
    ca:setName(s_CameraNames[GAME_CAMERAS_ORDER.CAMERA_WORLD_3D_SKYBOX])
    ca:setCameraFlag(s_CF[GAME_LAYER.LAYER_BACKGROUND])
    ca:setPosition3D(cc.vec3(0.0, 0.0, 50.0))
    self._worldScene:addChild(ca)
    --create a camera to look the 3D models in world 3D scene
    self._gameCameras[GAME_CAMERAS_ORDER.CAMERA_WORLD_3D_SCENE] = cc.Camera:createPerspective(60,
                                      visibleSize.width/visibleSize.height,
                                      0.1,
                                      200)
    ca = self._gameCameras[GAME_CAMERAS_ORDER.CAMERA_WORLD_3D_SCENE]
    ca:setDepth(GAME_CAMERAS_ORDER.CAMERA_WORLD_3D_SCENE)
    ca:setName(s_CameraNames[GAME_CAMERAS_ORDER.CAMERA_WORLD_3D_SCENE])
    ca:setCameraFlag(s_CF[GAME_LAYER.LAYER_DEFAULT])
    self._worldScene:addChild(ca)

    -- create 3D objects and add to world scene
    self:createWorld3D()
    self._worldScene:addChild(self._skyBox)
    self._worldScene:addChild(self._terrain)
    self._worldScene:addChild(self._player)
    self._worldScene:addChild(self._monsters[1])
    self._worldScene:addChild(self._monsters[2])
    -- move camera above player
    local playerPos = self._player:getPosition3D()
    ca:setPosition3D(cc.vec3(playerPos.x, playerPos.y + 45, playerPos.z + 60))
    ca:setRotation3D(cc.vec3(-45,0,0))
    self._worldScene:setPosition3D(s_scenePositons[GAME_SCENE.SCENE_WORLD])
    self:addChild(self._worldScene)

    --[[
    test scene is UI scene, use default camera
    use the default camera to look 2D base UI layer
    ]]--
    self._gameCameras[GAME_CAMERAS_ORDER.CAMERA_UI_2D] = self:getDefaultCamera()
    ca = self._gameCameras[GAME_CAMERAS_ORDER.CAMERA_UI_2D]
    ca:setDepth(GAME_CAMERAS_ORDER.CAMERA_UI_2D)
    ca:setName(s_CameraNames[GAME_CAMERAS_ORDER.CAMERA_UI_2D])
    --create UI element and add to ui scene
    self:createUI()
    self:addChild(self._ui)


    --create dialog scene, this scene has two dialog and three cameras
    self._dlgScene = cc.Node:create()
    --use default camera to render the base 2D elements
    self._gameCameras[GAME_CAMERAS_ORDER.CAMERA_DIALOG_2D_BASE] = cc.Camera:create()
    ca = self._gameCameras[GAME_CAMERAS_ORDER.CAMERA_DIALOG_2D_BASE]
    ca:setDepth(GAME_CAMERAS_ORDER.CAMERA_DIALOG_2D_BASE)
    ca:setName(s_CameraNames[GAME_CAMERAS_ORDER.CAMERA_DIALOG_2D_BASE])
    self._dlgScene:addChild(ca)
    --create a camera to look the 3D model in dialog scene
    self._gameCameras[GAME_CAMERAS_ORDER.CAMERA_DIALOG_3D_MODEL] = cc.Camera:create()
    ca = self._gameCameras[GAME_CAMERAS_ORDER.CAMERA_DIALOG_3D_MODEL]
    ca:setDepth(GAME_CAMERAS_ORDER.CAMERA_DIALOG_3D_MODEL)
    ca:setName(s_CameraNames[GAME_CAMERAS_ORDER.CAMERA_DIALOG_3D_MODEL])
    ca:setCameraFlag(s_CF[GAME_LAYER.LAYER_MIDDLE])
    self._dlgScene:addChild(ca)
    --create a camera to look the UI element over on the 3D models
    self._gameCameras[GAME_CAMERAS_ORDER.CAMERA_DIALOG_2D_ABOVE] = cc.Camera:create()
    ca = self._gameCameras[GAME_CAMERAS_ORDER.CAMERA_DIALOG_2D_ABOVE]
    ca:setDepth(GAME_CAMERAS_ORDER.CAMERA_DIALOG_2D_ABOVE)
    ca:setName(s_CameraNames[GAME_CAMERAS_ORDER.CAMERA_DIALOG_2D_ABOVE])
    ca:setCameraFlag(s_CF[GAME_LAYER.LAYER_TOP])
    self._dlgScene:addChild(ca)
    --create dialogs and add to dialog scene
    self:createPlayerDlg()
    self._dlgScene:addChild(self._playerDlg)
    self:createDetailDlg()
    self._dlgScene:addChild(self._detailDlg)
    --add dialog scene to test scene, which can't see the other element
    self._dlgScene:setPosition3D(s_scenePositons[GAME_SCENE.SCENE_DIALOG])
    self:addChild(self._dlgScene)

    --create description scene, this scene has a dialog and three cameras
    self._osdScene = cc.Node:create()
    --use default camera for render 2D element
    self._gameCameras[GAME_CAMERAS_ORDER.CAMERA_OSD_2D_BASE] = cc.Camera:create()
    ca = self._gameCameras[GAME_CAMERAS_ORDER.CAMERA_OSD_2D_BASE] 
    ca:setDepth(GAME_CAMERAS_ORDER.CAMERA_OSD_2D_BASE)
    ca:setName(s_CameraNames[GAME_CAMERAS_ORDER.CAMERA_OSD_2D_BASE])
    self._osdScene:addChild(ca)
    --create a camera to look the 3D model in dialog scene
    self._gameCameras[GAME_CAMERAS_ORDER.CAMERA_OSD_3D_MODEL] = cc.Camera:create()
    ca = self._gameCameras[GAME_CAMERAS_ORDER.CAMERA_OSD_3D_MODEL]
    ca:setDepth(GAME_CAMERAS_ORDER.CAMERA_OSD_3D_MODEL)
    ca:setName(s_CameraNames[GAME_CAMERAS_ORDER.CAMERA_OSD_3D_MODEL])
    ca:setCameraFlag(s_CF[GAME_LAYER.LAYER_MIDDLE])
    self._osdScene:addChild(ca)
    --create a camera to look the UI element over on the 3D models
    self._gameCameras[GAME_CAMERAS_ORDER.CAMERA_OSD_2D_ABOVE] = cc.Camera:create()
    ca = self._gameCameras[GAME_CAMERAS_ORDER.CAMERA_OSD_2D_ABOVE]
    ca:setDepth(GAME_CAMERAS_ORDER.CAMERA_OSD_2D_ABOVE)
    ca:setName(s_CameraNames[GAME_CAMERAS_ORDER.CAMERA_OSD_2D_ABOVE])
    ca:setCameraFlag(s_CF[GAME_LAYER.LAYER_TOP])
    self._osdScene:addChild(ca)
    --create desc dialog and add to osd scene
    self:createDescDlg()
    self._osdScene:addChild(self._descDlg)
    --add osd scene to test scene, which can't see the other elements
    self._osdScene:setPosition3D(s_scenePositons[GAME_SCENE.SCENE_OSD])
    self:addChild(self._osdScene)

    self._playerDlg:setVisible(false)
    self._detailDlg:setVisible(false)
    self._descDlg:setVisible(false)

    --add touch envent callback
    local listener = cc.EventListenerTouchOneByOne:create()
    listener:registerScriptHandler(function (touch, event)
        return true
    end,cc.Handler.EVENT_TOUCH_BEGAN)

    listener:registerScriptHandler(function (touch, event)
        local location = touch:getLocation()
        local camera = self._gameCameras[GAME_CAMERAS_ORDER.CAMERA_WORLD_3D_SCENE]
        if camera ~= cc.Camera:getVisitingCamera() then
            return
        end

        if nil ~= self._player then
            local nearP = cc.vec3(location.x, location.y, 0.0)
            print("nearP is ", nearP.x, " ", nearP.y, " ", nearP.z)
            local farP  = cc.vec3(location.x, location.y, 1.0)
            print("farP is ", farP.x, " ", farP.y, " ", farP.z)
            --first, convert screen touch location to the world location on near and far plane
            local size = cc.Director:getInstance():getWinSize()
            nearP = camera:unprojectGL(size, nearP, nearP)
            farP  = camera:unprojectGL(size, farP, farP)
            print("before farP is ", farP.x, farP.y, farP.z)
            --second, convert world location to terrain's local position on near/far plane
            local worldToNodeMat = self._terrain:getNodeToWorldTransform()
            worldToNodeMat = cc.mat4.getInversed(worldToNodeMat)
            print("worldToNodeMat is ", worldToNodeMat[1], " ", worldToNodeMat[2], " ", worldToNodeMat[3], " ", worldToNodeMat[4], " ", worldToNodeMat[5], " ", worldToNodeMat[6], " ", worldToNodeMat[7], " ", worldToNodeMat[8], " ", worldToNodeMat[9], " ", worldToNodeMat[10], " ", worldToNodeMat[11], " ", worldToNodeMat[12], " ", worldToNodeMat[13], " ", worldToNodeMat[14], " ", worldToNodeMat[15], " ", worldToNodeMat[16], " ")
            nearP = mat4_transformVector(worldToNodeMat, nearP.x, nearP.y, nearP.z, 1.0, nearP)
            print("nearP is ",nearP.x, nearP.y, nearP.z)
            farP = mat4_transformVector(worldToNodeMat, farP.x, farP.y, farP.z, 1.0, farP)
            print("farP is ",farP.x, farP.y, farP.z)
            local dir = cc.vec3(farP.x - nearP.x, farP.y - nearP.y, farP.z - nearP.z)
            dir = cc.vec3normalize(dir)

            local rayStep = cc.vec3(15 * dir.x, 15 * dir.y, 15 * dir.z)
            local rayPos =  nearP
            local rayStartPosition = nearP
            local lastRayPosition  = rayPos
            rayPos = cc.vec3(rayPos.x + rayStep.x, rayPos.y + rayStep.y, rayPos.z + rayStep.z)
            -- Linear search - Loop until find a point inside and outside the terrain Vector3 
            local height = self._terrain:getHeight(rayPos.x, rayPos.z)

            while rayPos.y > height do
                lastRayPosition = rayPos 
                rayPos = cc.vec3(rayPos.x + rayStep.x, rayPos.y + rayStep.y, rayPos.z + rayStep.z)
                height = self._terrain:getHeight(rayPos.x,rayPos.z) 
            end

            local startPosition = lastRayPosition
            local endPosition   = rayPos

            for i = 1, 32 do
                -- Binary search pass 
                local middlePoint = cc.vec3(0.5 * (startPosition.x + endPosition.x), 0.5 * (startPosition.y + endPosition.y), 0.5 * (startPosition.z + endPosition.z))
                if (middlePoint.y < height) then
                    endPosition = middlePoint 
                else 
                    startPosition = middlePoint
                end
            end

            local collisionPoint = cc.vec3(0.5 * (startPosition.x + endPosition.x), 0.5 * (startPosition.y + endPosition.y), 0.5 * (startPosition.z + endPosition.z))
            local playerPos = self._player:getPosition3D()
            dir = cc.vec3(collisionPoint.x - playerPos.x, collisionPoint.y - playerPos.y, collisionPoint.z - playerPos.z)
            dir.y = 0
            dir = cc.vec3normalize(dir)
            self._player._headingAngle =  -1 * math.acos(-dir.z)

            self._player._headingAxis = vec3_cross(dir, cc.vec3(0, 0, -1), self._player._headingAxis)
            self._player._targetPos = collisionPoint
            self._player._playerState = PLAER_STATE.FORWARD
        end
        event:stopPropagation()
    end,cc.Handler.EVENT_TOUCH_ENDED)
    local eventDispatcher = self:getEventDispatcher()
    eventDispatcher:addEventListenerWithSceneGraphPriority(listener, self)
end

function Scene3DTestScene:createWorld3D()
    --create skybox
    --create and set our custom shader
    local shader = cc.GLProgram:createWithFilenames("Sprite3DTest/cube_map.vert",
                                                 "Sprite3DTest/cube_map.frag")
    local state = cc.GLProgramState:create(shader)
    
    -- create the second texture for cylinder
    self._textureCube = cc.TextureCube:create("Sprite3DTest/skybox/left.jpg",
                                       "Sprite3DTest/skybox/right.jpg",
                                       "Sprite3DTest/skybox/top.jpg",
                                       "Sprite3DTest/skybox/bottom.jpg",
                                       "Sprite3DTest/skybox/front.jpg",
                                       "Sprite3DTest/skybox/back.jpg")

    --set texture parameters
    local tRepeatParams = { magFilter = gl.LINEAR , minFilter = gl.LINEAR , wrapS = gl.MIRRORED_REPEAT  , wrapT = gl.MIRRORED_REPEAT }
    self._textureCube:setTexParameters(tRepeatParams)

    --pass the texture sampler to our custom shader
    state:setUniformTexture("u_cubeTex", self._textureCube)

    --add skybox
    self._skyBox = cc.Skybox:create()
    self._skyBox:setCameraMask(s_CM[GAME_LAYER.LAYER_BACKGROUND])
    self._skyBox:setTexture(self._textureCube)
    self._skyBox:setScale(700.0)

    --create terrain
    local detailMapR = { _detailMapSrc = "TerrainTest/dirt.jpg", _detailMapSize = 35}
    local detailMapG = { _detailMapSrc = "TerrainTest/Grass2.jpg", _detailMapSize = 10}
    local detailMapB = { _detailMapSrc = "TerrainTest/road.jpg", _detailMapSize = 35}
    local detailMapA = { _detailMapSrc = "TerrainTest/GreenSkin.jpg", _detailMapSize = 20}
    local terrainData = { _heightMapSrc = "TerrainTest/heightmap16.jpg", _alphaMapSrc = "TerrainTest/alphamap.png" , _detailMaps = {detailMapR, detailMapG, detailMapB, detailMapA}, _detailMapAmount = 4, _mapHeight = 40.0, _mapScale = 2.0 }

    self._terrain = cc.Terrain:create(terrainData,cc.Terrain.CrackFixedType.SKIRT)
    self._terrain:setMaxDetailMapAmount(4)
    self._terrain:setDrawWire(false)

    self._terrain:setSkirtHeightRatio(3)
    self._terrain:setLODDistance(64,128,192)

    --create player
    self._player = Player:create("Sprite3DTest/girl.c3b", self._gameCameras[GAME_CAMERAS_ORDER.CAMERA_WORLD_3D_SCENE], self._terrain)
    self._player:setScale(0.08)
    self._player:setPositionY(self._terrain:getHeight(self._player:getPositionX(), self._player:getPositionZ()))

    local animation = cc.Animation3D:create("Sprite3DTest/girl.c3b","Take 001")
    if nil ~= animation then 
        local animate = cc.Animate3D:create(animation)
        self._player:runAction(cc.RepeatForever:create(animate))
    end

    --add Particle3D for test blend
    local rootps = cc.PUParticleSystem3D:create("Particle3D/scripts/mp_torch.pu")
    rootps:setScale(2)
    rootps:setPosition3D(cc.vec3(0, 150, 0))
    local moveby = cc.MoveBy:create(2.0, cc.p(50.0, 0.0))
    local moveby1 = cc.MoveBy:create(2.0, cc.p(-50.0, 0.0))
    rootps:runAction(cc.RepeatForever:create(cc.Sequence:create(moveby, moveby1)))
    rootps:startParticleSystem()
    self._player:addChild(rootps, 0)

    --add BillBoard for test blend
    local billboard = cc.BillBoard:create("Images/btn-play-normal.png")
    billboard:setPosition3D(cc.vec3(0,180,0))
    self._player:addChild(billboard)

    -- create two Sprite3D monster, one is transparent
    local monster = cc.Sprite3D:create("Sprite3DTest/orc.c3b")
    local playerPos = self._player:getPosition3D()
    monster:setRotation3D(cc.vec3(0,180,0))
    monster:setPosition3D(cc.vec3(playerPos.x + 50, playerPos.y - 10, playerPos.z))
    monster:setOpacity(128)
    self._monsters[1] = monster
    monster = cc.Sprite3D:create("Sprite3DTest/orc.c3b")
    monster:setRotation3D(cc.vec3(0,180,0))
    monster:setPosition3D(cc.vec3(playerPos.x - 50, playerPos.y - 5, playerPos.z))
    self._monsters[2] = monster

    local targetPlatform = cc.Application:getInstance():getTargetPlatform()
    if targetPlatform == cc.PLATFORM_OS_ANDROID  or targetPlatform == cc.PLATFORM_OS_WINRT  or targetPlatform == cc.PLATFORM_OS_WP8  then
        self._backToForegroundListener = cc.EventListenerCustom:create("event_renderer_recreated", function (eventCustom)

                local state = self._skyBox:getGLProgramState()
                local glProgram = state:getGLProgram()
                glProgram:reset()
                glProgram:initWithFilenames("Sprite3DTest/cube_map.vert", "Sprite3DTest/cube_map.frag")
                glProgram:link()
                glProgram:updateUniforms()
                
                self._textureCube:reloadTexture()
                
                local tRepeatParams = { magFilter = gl.NEAREST , minFilter = gl.NEAREST , wrapS = gl.MIRRORED_REPEAT  , wrapT = gl.MIRRORED_REPEAT }
                self._textureCube:setTexParameters(tRepeatParams)
                state:setUniformTexture("u_cubeTex", self._textureCube)
                
                self._skyBox:reload()
                self._skyBox:setTexture(self._textureCube)
        end)
        cc.Director:getInstance():getEventDispatcher():addEventListenerWithFixedPriority(self._backToForegroundListener, 1)
    end    
end

function Scene3DTestScene:createCameraButton(tag, text)
    local cb = ccui.CheckBox:create("cocosui/check_box_normal.png",
                                   "cocosui/check_box_normal_press.png",
                                   "cocosui/check_box_active.png",
                                   "cocosui/check_box_normal_disable.png",
                                   "cocosui/check_box_active_disable.png")
    cb:setTag(tag)
    cb:setSelected(true)
    if nil ~= text then
        cb:setName(text)
    end
    cb:setAnchorPoint(cc.p(0, 0.5))
    cb:setScale(0.8)
    cb:addClickEventListener(function(sender)
        local index = sender:getTag()
        print("index", index)
        local camera = self._gameCameras[index]
        camera:setVisible(not camera:isVisible()) 
    end)
    if nil ~= text then
        local label = ccui.Text:create()
        label:setString(text)
        label:setAnchorPoint(cc.p(0, 0))
        label:setPositionX(cb:getContentSize().width)
        cb:addChild(label)
    end
    return cb
end

function Scene3DTestScene:createUI()
    self._ui = cc.Layer:create()
    --add player button
    local showPlayerDlgItem = cc.MenuItemImage:create("Images/Pea.png", "Images/Pea.png")
    showPlayerDlgItem:registerScriptTapHandler(function (tag, sender)
        if nil ~= self._playerDlg then
            self._playerDlg:setVisible(not self._playerDlg:isVisible())
        end
    end)
    showPlayerDlgItem:setName("showPlayerDlgItem")
    showPlayerDlgItem:setPosition(VisibleRect:left().x + 30, VisibleRect:top().y - 30)

    local ttfConfig = { fontFilePath = "fonts/arial.ttf", fontSize = 20}
    local descItem  = cc.MenuItemLabel:create(cc.Label:createWithTTF(ttfConfig, "Description"))
    descItem:registerScriptTapHandler(function(tag, sender)
        if self._descDlg:isVisible() then
            --hide descDlg
            self._descDlg:setVisible(false)
        else
            --animate show descDlg
            self._descDlg:setVisible(true)
            self._descDlg:setScale(0)
            self._descDlg:runAction(cc.ScaleTo:create(2, 1.0))
        end
    end)
    descItem:setName("descItem")
    descItem:setPosition(cc.p(VisibleRect:right().x - 50, VisibleRect:top().y - 25))

    local menu = cc.Menu:create(showPlayerDlgItem, descItem)
    menu:setPosition(cc.p(0.0, 0.0))
    self._ui:addChild(menu)

    --second, add cameras control button to ui
    local pos = VisibleRect:leftBottom()
    pos.y = pos.y + 30
    local stepY = 25

    for i = 1, GAME_CAMERAS_ORDER.CAMERA_COUNT - 1 do
        if i ~= GAME_CAMERAS_ORDER.CAMERA_UI_2D then
            local cameraBtn = self:createCameraButton(i, s_CameraNames[i])
            cameraBtn:setPosition(pos)
            self._ui:addChild(cameraBtn)
            pos.y = pos.y + stepY
        end
    end

    local labelCameras = ccui.Text:create()
    labelCameras:setString("Cameras")
    labelCameras:setAnchorPoint(cc.p(0, 0))
    labelCameras:setPosition(pos)
    self._ui:addChild(labelCameras)
end

function Scene3DTestScene:createPlayerDlg()
    cc.SpriteFrameCache:getInstance():addSpriteFrames(s_s9s_ui_plist)
    
    local dlgSize = cc.size(190, 240)
    local pos = VisibleRect:center()
    local margin = 10

    --first, create dialog ui part, include background, title and buttons
    self._playerDlg = ccui.Scale9Sprite:createWithSpriteFrameName("button_actived.png")
    self._playerDlg:setContentSize(dlgSize)
    self._playerDlg:setAnchorPoint(cc.p(1, 0.5))
    pos.y = pos.y - margin
    pos.x = pos.x - margin
    self._playerDlg:setPosition(pos)

    --title
    local title = cc.Label:createWithTTF("Player Dialog","fonts/arial.ttf",16)
    title:setPosition(dlgSize.width / 2, dlgSize.height - margin * 2)
    self._playerDlg:addChild(title)

    --player background
    local bgSize = cc.size(110, 180)
    local bgPos  = cc.p(margin, dlgSize.height / 2 - margin)
    local playerBg = ccui.Scale9Sprite:createWithSpriteFrameName("item_bg.png")
    playerBg:setContentSize(bgSize)
    playerBg:setAnchorPoint(cc.p(0, 0.5))
    playerBg:setPosition(bgPos)
    self._playerDlg:addChild(playerBg)

    --item background and item
    local itemSize   = cc.size(48, 48)
    local itemAnchor = cc.p(0, 1)
    local itemPos    = cc.p(bgPos.x + bgSize.width + margin, bgPos.y + bgSize.height / 2)
    local itemBg = ccui.Scale9Sprite:createWithSpriteFrameName("item_bg.png")
    itemBg:setContentSize(itemSize)
    itemBg:setAnchorPoint(itemAnchor)
    itemBg:setPosition(itemPos)
    self._playerDlg:addChild(itemBg)

    local item = ccui.Button:create("crystal.png", "", "", ccui.TextureResType.plistType)
    item:setTitleText("crystal")
    item:setScale(1.5)
    item:setAnchorPoint(itemAnchor)
    item:setPosition(itemPos)
    item:addClickEventListener(function(sender)
        if self._detailDlg ~= nil then
            self._detailDlg:setVisible(not self._detailDlg:isVisible())
        end
    end)
    self._playerDlg:addChild(item)

    --second, add 3d actor, which on dialog layer
    local girl = cc.Sprite3D:create("Sprite3DTest/girl.c3b")
    girl:setScale(0.5)
    girl:setPosition(bgSize.width / 2, margin * 2)
    girl:setCameraMask(s_CM[GAME_LAYER.LAYER_MIDDLE])
    playerBg:addChild(girl)

    --third, add zoom in/out button, which is 2d ui element and over 3d actor
    local zoomIn = ccui.Button:create("cocosui/animationbuttonnormal.png","cocosui/animationbuttonpressed.png")
    zoomIn:setScale(0.5)
    zoomIn:setAnchorPoint(cc.p(1, 1))
    zoomIn:setPosition(cc.p(bgSize.width / 2 - margin / 2, bgSize.height - margin))
    zoomIn:addClickEventListener(function(sender)
        girl:setScale(girl:getScale() * 2)
    end)
    zoomIn:setTitleText("Zoom In")
    zoomIn:setName("Zoom In")
    zoomIn:setCameraMask(s_CM[GAME_LAYER.LAYER_TOP])
    playerBg:addChild(zoomIn)

    local zoomOut = ccui.Button:create("cocosui/animationbuttonnormal.png", "cocosui/animationbuttonpressed.png")
    zoomOut:setScale(0.5)
    zoomOut:setAnchorPoint(cc.p(0, 1))
    zoomOut:setPosition(cc.p(bgSize.width / 2 + margin / 2, bgSize.height - margin))
    zoomOut:addClickEventListener(function(sender)
        girl:setScale(girl:getScale() / 2)
    end)
    zoomOut:setTitleText("Zoom Out")
    zoomOut:setName("Zoom Out")
    zoomOut:setCameraMask(s_CM[GAME_LAYER.LAYER_TOP])
    playerBg:addChild(zoomOut)

    --forth, add slider bar
    local slider = ccui.Slider:create("cocosui/slidbar.png", "cocosui/sliderballnormal.png")
    slider:setScale9Enabled(true)
    slider:setPosition(cc.p(bgSize.width / 2, margin))
    slider:setContentSize(cc.size(bgSize.width - margin, slider:getContentSize().height))
    slider:addEventListener(function (sender, type)
        girl:setRotation3D(cc.vec3(0, 360 * slider:getPercent() / 100, 0))
    end)
    slider:setName("Slider")
    slider:setCameraMask(s_CM[GAME_LAYER.LAYER_TOP])
    playerBg:addChild(slider)
end

function Scene3DTestScene:createDetailDlg()
    cc.SpriteFrameCache:getInstance():addSpriteFrames(s_s9s_ui_plist)
    
    local dlgSize = cc.size(190, 240)
    local pos = VisibleRect:center()
    local margin = 10
    
    --create dialog
    self._detailDlg = ccui.Scale9Sprite:createWithSpriteFrameName("button_actived.png")
    self._detailDlg:setContentSize(dlgSize)
    self._detailDlg:setAnchorPoint(cc.p(0, 0.5))
    self._detailDlg:setOpacity(224)
    pos.y = pos.y  - margin
    pos.x = pos.x  + margin
    self._detailDlg:setPosition(pos)

    -- title
    local title = cc.Label:createWithTTF("Detail Dialog","fonts/arial.ttf",16)
    title:setPosition(dlgSize.width / 2, dlgSize.height - margin * 2)
    self._detailDlg:addChild(title)
    
    -- add a spine ffd animation on it
    local skeletonNode = sp.SkeletonAnimation:create("spine/goblins-ffd.json", "spine/goblins-ffd.atlas", 1.5)
    skeletonNode:setAnimation(0, "walk", true)
    skeletonNode:setSkin("goblin")

    skeletonNode:setScale(0.25)
    local windowSize = cc.Director:getInstance():getWinSize()
    skeletonNode:setPosition(cc.p(dlgSize.width / 2, 20))
    self._detailDlg:addChild(skeletonNode)
end

function Scene3DTestScene:createDescDlg()
    cc.SpriteFrameCache:getInstance():addSpriteFrames(s_s9s_ui_plist)
    
    local dlgSize = cc.size(440, 240)
    local pos = VisibleRect:center()
    local margin = 10
    
    --create dialog
    self._descDlg = ccui.Scale9Sprite:createWithSpriteFrameName("button_actived.png")
    self._descDlg:setContentSize(dlgSize)
    self._descDlg:setOpacity(224)
    self._descDlg:setPosition(pos)

    --title
    local title = cc.Label:createWithTTF("Description Dialog","fonts/arial.ttf",16)
    title:setPosition(dlgSize.width / 2, dlgSize.height - margin * 2)
    self._descDlg:addChild(title)

    --add a label to retain description text
    local textSize = cc.size(400, 220)
    local textPos  = cc.p(margin, dlgSize.height - (20 + margin))
    local desc =  "Scene 3D test for 2D and 3D mix rendering.\n" ..
    "\n" ..
    "- Game world composite with terrain, skybox and 3D objects.\n" ..
    "- UI composite with 2D nodes.\n" ..
    "- Click the icon at the topleft conner, will show a player dialog which" ..
    "there is a 3D sprite on it.\n" ..
    "- There are two button to zoom the player model, which should keep above" ..
    "on 3D model.\n" ..
    "- This description dialog should above all other elements.\n" ..
    "\n" ..
    "Click \"Description\" button to hide this dialog.\n"

    local text = cc.Label:createWithSystemFont(desc, "Helvetica", 12, textSize)
    text:setAnchorPoint(cc.p(0, 1))
    text:setPosition(margin, dlgSize.height - (20 + margin))
    self._descDlg:addChild(text)
    
    --second, add a 3D model
    local fileName = "Sprite3DTest/ReskinGirl.c3b"
    local girlPos = cc.p(textPos.x + textSize.width - 40, margin)
    self._reskinGirl = cc.Sprite3D:create(fileName)
    self._reskinGirl:setCameraMask(s_CM[GAME_LAYER.LAYER_MIDDLE])
    self._reskinGirl:setScale(2.5)
    self._reskinGirl:setPosition(girlPos)
    self._descDlg:addChild(self._reskinGirl)
    local animation = cc.Animation3D:create(fileName)
    if nil ~= animation then
        local animate = cc.Animate3D:create(animation)
        
        self._reskinGirl:runAction(cc.RepeatForever:create(animate))
    end

    local body = {"Girl_UpperBody01", "Girl_UpperBody02"}
    self._skins[SkinType.UPPER_BODY] = body

    local pants = {"Girl_LowerBody01", "Girl_LowerBody02"}
    self._skins[SkinType.PANTS] = pants

    local shoes = {"Girl_Shoes01", "Girl_Shoes02"}
    self._skins[SkinType.SHOES] = shoes

    local hair = {"Girl_Hair01", "Girl_Hair02"}
    self._skins[SkinType.HAIR] = hair

    local face = {"Girl_Face01", "Girl_Face02"}
    self._skins[SkinType.FACE] = face

    local hand = {"Girl_Hand01", "Girl_Hand02"}
    self._skins[SkinType.HAND] = hand

    local glasses = {"", "Girl_Glasses01"}
    self._skins[SkinType.GLASSES] = glasses

    for i = 1, SkinType.MAX_TYPE - 1 do
        self._curSkin[i] = 0
    end

    local function applyCurSkin()
        local meshCount = self._reskinGirl:getMeshCount()
        for i = 1, meshCount do
            local mesh = self._reskinGirl:getMeshByIndex(i - 1)
            local isVisible = false
            for j = 1, SkinType.MAX_TYPE - 1 do
                if mesh:getName() == self._skins[j][self._curSkin[j] + 1] then
                    isVisible = true
                    break
                end
            end

            self._reskinGirl:getMeshByIndex(i - 1):setVisible(isVisible)
        end
    end

    applyCurSkin()

    --third, add reskin buttons above 3D model
    local btnTexts =
    {
        "Hair",
        "Glasses",
        "Face",
        "Coat",
        "Hand",
        "Pants",
        "Shoes",
    }

    local btnPos = cc.p(dlgSize.width - margin, margin)
    for i = 1, SkinType.MAX_TYPE - 1 do
        local btn = ccui.Button:create("cocosui/animationbuttonnormal.png",
                                      "cocosui/animationbuttonpressed.png")
        btn:setScale(0.5)
        btn:setTag(i)
        btn:setAnchorPoint(cc.p(1, 0))
        btn:setPosition(btnPos)
        btnPos.y = btnPos.y + 20
        btn:addClickEventListener(function(sender)
            local index = sender:getTag()
            if index < SkinType.MAX_TYPE then
                self._curSkin[index] = (self._curSkin[index] + 1) % (#self._skins[index])
                applyCurSkin()
            end
        end)

        btn:setTitleText(btnTexts[i])
        btn:setCameraMask(s_CM[GAME_LAYER.LAYER_TOP])
        self._descDlg:addChild(btn)
    end
end

function Scene3DTestScene:onExit()
    local targetPlatform = cc.Application:getInstance():getTargetPlatform()
    if targetPlatform == cc.PLATFORM_OS_ANDROID  or targetPlatform == cc.PLATFORM_OS_WINRT  or targetPlatform == cc.PLATFORM_OS_WP8  then
        cc.Director:getInstance():getEventDispatcher():removeEventListener(self._backToForegroundListener)
    end
end



function Scene3DTestMain()
    local scene = Scene3DTestScene.create()
    scene:addChild(CreateBackMenuItem())

    return scene
end
