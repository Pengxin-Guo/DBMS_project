#!/bin/bash
sed -i -e 's/printf/DBG/g' ./master/master.c               #将master.c中的printf全部改为DBG
