-- phpMyAdmin SQL Dump
-- version 4.6.5.2
-- https://www.phpmyadmin.net/
--
-- Host: 192.168.10.111
-- Generation Time: 24 Ian 2019 la 16:36
-- Versiune server: 10.0.34-MariaDB-0ubuntu0.16.04.1
-- PHP Version: 7.0.32-0ubuntu0.16.04.1

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;

--
-- Database: `powermeter`
--

-- --------------------------------------------------------

--
-- Structura de tabel pentru tabelul `data`
--

CREATE TABLE `data` (
  `id` int(11) NOT NULL,
  `time` datetime NOT NULL,
  `senz1` smallint(6) NOT NULL,
  `senz2` smallint(6) NOT NULL,
  `senz3` smallint(6) NOT NULL,
  `senz4` smallint(6) NOT NULL,
  `senz5` smallint(6) NOT NULL,
  `senz6` smallint(6) NOT NULL,
  `senz7` smallint(6) NOT NULL,
  `senz8` smallint(6) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Indexes for dumped tables
--

--
-- Indexes for table `data`
--
ALTER TABLE `data`
  ADD PRIMARY KEY (`id`);

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
