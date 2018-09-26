/*
Navicat MySQL Data Transfer

Source Server         : LocalMySQL
Source Server Version : 50624
Source Host           : localhost:3306
Source Database       : maoding_test

Target Server Type    : MYSQL
Target Server Version : 50624
File Encoding         : 65001

Date: 2017-11-15 14:22:18
*/

SET FOREIGN_KEY_CHECKS=0;

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
