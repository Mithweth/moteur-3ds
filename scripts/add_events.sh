#!/bin/bash -u
while read c; do
  [ -z "$c" ] && continue
  [ "${c:0:1}" == "#" ] && continue
  key=$(cut -d= -f1 <<< "$c")
  room=$(cut -d_ -f1 <<< "$key")
  room_file="resources/rooms/$(tr 'A-Z' 'a-z' <<< "$room")/room"
  if [ ! -f "$room_file" ]; then
  	echo "room file $room_file not found"
  	continue
  fi
  i=2
  obj=
  while [[ ! "$(cut -d_ -f$i <<< "$key")" =~ EXAMINE.* ]]; do
  	obj="${obj}_$(cut -d_ -f$i <<< "$key")"
  	i=$((i + 1))
  done
  obj=$(cut -c2- <<< "$obj")
  echo "room: $room, hotspot: $obj, key: $key"
  if ! grep -q "HOTSPOT ${room}_${obj} " "$room_file"; then
  	echo "hotspot ${room}_$obj not found"
  	continue
  fi
  sed -i "/HOTSPOT ${room}_${obj} /a\ \tMESSAGE $key" "$room_file"
done < "$1"
