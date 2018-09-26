/*
Navicat MySQL Data Transfer

Source Server         : LocalMySQL
Source Server Version : 50624
Source Host           : localhost:3306
Source Database       : maoding_test

Target Server Type    : MYSQL
Target Server Version : 50624
File Encoding         : 65001

Date: 2017-11-15 14:21:59
*/

SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for maoding_storage_dir
-- ----------------------------
DROP TABLE IF EXISTS `maoding_storage_dir`;
CREATE TABLE `maoding_storage_dir` (
  `id` char(32) NOT NULL COMMENT '唯一编号',
  `deleted` tinyint(1) unsigned NOT NULL DEFAULT '0' COMMENT '删除标志',
  `create_time` timestamp NULL DEFAULT NULL ON UPDATE CURRENT_TIMESTAMP COMMENT '记录创建时间',
  `last_modify_time` timestamp NULL DEFAULT NULL ON UPDATE CURRENT_TIMESTAMP COMMENT '记录最后修改时间',
  `last_modify_user_id` char(32) DEFAULT NULL COMMENT '记录最后修改者用户id',
  `last_modify_post_id` char(32) DEFAULT NULL COMMENT '记录最后修改者职责id',
  `user_id` char(32) DEFAULT NULL COMMENT '目录创建者id,自动生成的没有创建者',
  `org_id` char(32) DEFAULT NULL COMMENT '目录创建组织id,项目目录在立项时创建，创建组织为立项组织',
  `project_id` char(32) DEFAULT NULL COMMENT '项目id,目录如果不属于任何一个项目，项目id为空',
  `task_id` char(32) DEFAULT NULL COMMENT '特定任务id',
  `type` smallint(4) unsigned DEFAULT NULL COMMENT '目录类别，如：系统默认目录、用户添加目录',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
