/*
Navicat MySQL Data Transfer

Source Server         : LocalMySQL
Source Server Version : 50624
Source Host           : localhost:3306
Source Database       : maoding_test

Target Server Type    : MYSQL
Target Server Version : 50624
File Encoding         : 65001

Date: 2017-11-15 14:21:00
*/

SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for maoding_storage
-- ----------------------------
DROP TABLE IF EXISTS `maoding_storage`;
CREATE TABLE `maoding_storage` (
  `id` char(32) NOT NULL COMMENT '唯一编号',
  `deleted` tinyint(1) unsigned NOT NULL DEFAULT '0' COMMENT '删除标志',
  `create_time` timestamp NULL DEFAULT NULL ON UPDATE CURRENT_TIMESTAMP COMMENT '记录创建时间',
  `last_modify_time` timestamp NULL DEFAULT NULL ON UPDATE CURRENT_TIMESTAMP COMMENT '记录最后修改时间',
  `last_modify_user_id` char(32) DEFAULT NULL COMMENT '记录最后修改者用户id',
  `last_modify_post_id` char(32) DEFAULT NULL COMMENT '记录最后修改者职责id',
  `type` smallint(4) unsigned NOT NULL DEFAULT '0' COMMENT '存储信息类型：0-文件，1-目录',
  `name` varchar(255) NOT NULL COMMENT '存储信息名称',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

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

-- ----------------------------
-- Table structure for maoding_storage_file
-- ----------------------------
DROP TABLE IF EXISTS `maoding_storage_file`;
CREATE TABLE `maoding_storage_file` (
  `id` char(32) NOT NULL COMMENT '唯一编号',
  `deleted` tinyint(1) unsigned NOT NULL DEFAULT '0' COMMENT '删除标志',
  `create_time` timestamp NULL DEFAULT NULL ON UPDATE CURRENT_TIMESTAMP COMMENT '记录创建时间',
  `last_modify_time` timestamp NULL DEFAULT NULL ON UPDATE CURRENT_TIMESTAMP COMMENT '记录最后修改时间',
  `last_modify_user_id` char(32) DEFAULT NULL COMMENT '记录最后修改者用户id',
  `last_modify_post_id` char(32) DEFAULT NULL COMMENT '记录最后修改者职责id',
  `scope` varchar(255) DEFAULT NULL COMMENT '在文件服务器上的存储位置',
  `key` varchar(255) DEFAULT NULL COMMENT '在文件服务器上的存储名称',
  `last_modify_ip` varchar(20) DEFAULT NULL COMMENT '最后上传的地址',
  `version` varchar(20) DEFAULT NULL COMMENT '用户自定义版本',
  `locking` tinyint(1) unsigned DEFAULT '0' COMMENT '是否锁定，0-不锁定，1-锁定',
  `downloading` smallint(4) unsigned DEFAULT '0' COMMENT '当前有多少用户正在下载',
  `uploaded_key` varchar(255) DEFAULT NULL COMMENT '已上传到文件服务器的新key值',
  `type` smallint(4) unsigned DEFAULT '0' COMMENT '文件类型',
  `md5` varchar(8) DEFAULT NULL,
  `file_size` bigint(16) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- ----------------------------
-- Table structure for maoding_storage_tree
-- ----------------------------
DROP TABLE IF EXISTS `maoding_storage_tree`;
CREATE TABLE `maoding_storage_tree` (
  `id` char(32) NOT NULL COMMENT '唯一编号',
  `deleted` tinyint(1) unsigned NOT NULL DEFAULT '0' COMMENT '删除标志',
  `create_time` timestamp NULL DEFAULT NULL ON UPDATE CURRENT_TIMESTAMP COMMENT '记录创建时间',
  `last_modify_time` timestamp NULL DEFAULT NULL ON UPDATE CURRENT_TIMESTAMP COMMENT '记录最后修改时间',
  `last_modify_user_id` char(32) DEFAULT NULL COMMENT '记录最后修改者用户id',
  `last_modify_post_id` char(32) DEFAULT NULL COMMENT '记录最后修改者职责id',
  `pid` char(32) DEFAULT NULL COMMENT '父节点在此表中的id',
  `path` varchar(255) NOT NULL COMMENT '从根节点到本节点的id路径，以","作为分隔符',
  `storage_id` char(32) NOT NULL COMMENT '对应的实体编号',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
