## About

This project is a monitoring application for btrfs file systems.  Basically it is run by crontab at regular intervals (Say, once per day), it calls some standard Linux applications such as `smartctl` and `btrfs device stats`, parses the output and prints some basic stats and whether the drive is happy or not to syslog.  You can then read the logs on the machine or read them after they are pushed to a central logging server.

## Requirements

- [libjson-c](https://github.com/json-c/json-c)

## Building

Install dependencies:

Ubuntu:
```bash
sudo apt install gcc-c++ cmake json-c-dev gtest-dev
```

Fedora:
```bash
sudo dnf install gcc-c++ cmake json-c-devel gtest-devel
```

Build:
```bash
cmake .
make
```

Install it:
```bash
sudo cp lumber-jill /usr/bin/lumber-jill
```

Create the output directory (You can change this, but you'll need to use a JSON settings file and specify where it goes):
```bash
sudo mkdir -p /root/.config/lumber-jill/
sudo cp ./settings.json /root/.config/lumber-jill/settings.json
```

Set the path to your BTRFS file system mount point in the settings file (Something like /data1):
```bash
sudo vi /root/.config/lumber-jill/settings.json
```

Test it can run and outputs to syslog correctly:
```bash
sudo lumber-jill
sudo tail -n 20 /var/log/messages
```

## Cron

Create a cron job to run it once per day:
1) 
```bash
sudo crontab -e
```
2) Add an entry to crontab to run lumber-jill [once per day at 3am](https://crontab.guru/#0_3_*_*_*):
```bash
0 3 * * * /usr/bin/lumber-jill
```
3) Check it was added:
```bash
sudo crontab -l
```

## Removal

Remove the lumber-jill entry from crontab:
```bash
sudo crontab -e
```

Remove the executable and settings files:
```bash
sudo rm /usr/bin/lumber-jill /root/.config/lumber-jill/settings.json
```

## Rational

1. By the time you are getting read/checksum errors in `dmesg` it is too late, so this utility helps get the "this drive is failing" message out as soon as possible
2. An easy way to monitor a system is to collect all the syslog on a central logging server, so we just output to syslog, it is up to the user to monitor the logs or push them to a central server for monitoring
3. Structured data in the syslog so that the central logging server can parse it, reason about it and graph it

## The Name

What, women can't be lumber jacks? Jill is the bestest, toughest, formidiblest lumber jack in Riverdale Falls, she's amazing at logging, chews jerky, spits and has an axe to grind. And she's all out of jerky.
