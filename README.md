# Telecommunications project, Group 24

This repository contains the documentation and source code for the telecommunications project of Group 24 (Autumn 2025, Oulu University of Applied Sciences)

## System components

The project consists of the following software/hardware components:

    - Nordic Semiconductor nRF5340-DK (Bluetooth Low Energy Development Kit)
    - Raspberry Pi 3B+ (Ubuntu GNU/Linux recommended)
    - MPU6050 accelerometer/gyroscope module (GY-521 development module recommended)

## Features

    - Transmission/reception of gyroscope, accelerometer and button data over BLE
        The nRF device reads data from the gyroscope and/or the accelerometer and transmits it over BLE.
        The server application receives the data and stores it in the database for further processing.

    - Analysis of received accelerometer data with K-means clustering

    - Graphical User Interface (GUI)
        The data is displayed through a Web-based GUI hosted with the Apache server software running on the Raspberry Pi 3B+
        Users can view real-time accelerometer data from a standard web browser.

    - Sensor self-testing
    	The MPU6050 driver has been equipped with sensor self-tests according to the specifications of the manufacturer.
	In the case of the self-test reporting a deviation of less than -14 percent or over 14 percent from factory trim values, the driver warns the user using the Zephyr WRN_LOG messages.


## Dependencies

    - nRF SDK

## System Diagram

<img width="461" height="361" alt="arkkitehtuuri drawio" src="arkkitehtuurikuva.png" />

## Credits

Luukas Ahola, Lauri Knuuttila
