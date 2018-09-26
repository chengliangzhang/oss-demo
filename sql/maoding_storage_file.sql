/*
Navicat MySQL Data Transfer

Source Server         : LocalMySQL
Source Server Version : 50624
Source Host           : localhost:3306
Source Database       : maoding_test

Target Server Type    : MYSQL
Target Server Version : 50624
File Encoding         : 65001

Date: 2017-11-15 14:22:06
*/

SET FOREIGN_KEY_CHECKS=0;

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
