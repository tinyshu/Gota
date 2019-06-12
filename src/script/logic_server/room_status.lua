local RoomState = {
	InView = 1,   -- 集结玩家
	Ready = 2,    --玩家集结完毕
	Start = 3,    -- 玩家都准备好了
	Playing = 4,  -- 在游戏中
	CheckOut = 5, -- 游戏结算
}

return RoomState