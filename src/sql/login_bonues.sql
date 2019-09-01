CREATE TABLE `login_bonues` (
`id`  int(10) UNSIGNED NOT NULL AUTO_INCREMENT ,
`uid`  int(11) NULL DEFAULT 0 COMMENT '用户在游戏的uid' ,
`bonues`  int(11) NOT NULL DEFAULT 0 COMMENT '明天奖励数量' ,
`status`  tinyint(4) NOT NULL DEFAULT 0 COMMENT '是否已经领取过 0未领取，1已领取' ,
`bonues_time`  int(11) NOT NULL COMMENT '发放奖励时间' ,
`days`  int(11) NOT NULL DEFAULT 0 COMMENT '连续登陆天数' ,
PRIMARY KEY (`id`),
UNIQUE INDEX `uid_idx` (`uid`) USING BTREE 
)
ENGINE=InnoDB
DEFAULT CHARACTER SET=utf8 COLLATE=utf8_general_ci
AUTO_INCREMENT=1
ROW_FORMAT=DYNAMIC
;

