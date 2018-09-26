/*
Navicat MySQL Data Transfer

Source Server         : LocalMySQL
Source Server Version : 50624
Source Host           : localhost:3306
Source Database       : maoding_test

Target Server Type    : MYSQL
Target Server Version : 50624
File Encoding         : 65001

Date: 2017-11-15 14:20:35
*/

SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for maoding_const
-- ----------------------------
DROP TABLE IF EXISTS `maoding_const`;
CREATE TABLE `maoding_const` (
  `classic_id` smallint(4) unsigned NOT NULL COMMENT '常量分类编号，0：分类类别',
  `value_id` smallint(4) unsigned NOT NULL COMMENT '特定分类内的常量编号',
  `content` varchar(255) NOT NULL COMMENT '常量的基本定义',
  `content_extra` varchar(255) DEFAULT NULL COMMENT '常量的扩展定义',
  PRIMARY KEY (`classic_id`,`value_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
