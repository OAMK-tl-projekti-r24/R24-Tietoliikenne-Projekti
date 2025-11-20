# Telecommunications project, Group 24

This repository contains the documentation and source code for the telecommunications project of Group 24 (Autumn 2025, Oulu University of Applied Sciences)

## System components

The project consists of the following software/hardware components:

    - Nordic Semiconductor nRF5340-DK (Bluetooth Low Energy Development Kit)
    - Raspberry Pi 3B+
    - Linux Ubuntu Server

## Features

    - Transmission/reception of gyroscope data over BLE
        The nRF device reads data from the gyroscope and transmits it over BLE.
        The server application receives this data and stores it in the database for further processing.

    - Analysis of received gyroscope data with K-means clustering

    - Graphical User Interface (GUI)
        Data is displayed through a web-based GUI hosted by Linux Ubuntu Server via Apache
        Users can view real-time gyroscope data from a standard web browser.


## Dependencies

    - nRF SDK

## System Diagram

<img width="461" height="361" alt="arkkitehtuuri drawio" src="arkkitehtuurikuva.png" />

## Credits

Luukas Ahola, Lauri Knuuttila
