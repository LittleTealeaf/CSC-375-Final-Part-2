## Notes

SSIDs will START with omega (omega-1323) etc...

IP will likely be `192.168.3.1:1883`

Can install mosquito and run it

### MQTT

- Distance: "CSC375/dist"
- Start / Stop: "CSC375/control"

- In the format <MAC><value>

message format:

"CSC375/control"

```json
{
    "MAC": string with ':',
    "control": int/long
}
```

"CSC375/dist"

```json
{
    "MAC": string with ':',
    "dist": int (truncate if necessary) 
}
```

```cpp
DynamicJsonContent doc(1024);



```

### Fire beetle

Report Mac Address and Distance If fire beetle is currently publishing distances, turn on the LED
If it is not, then turn off the LED

### Core 2

- Break up screen into 6 boxes
- Border
- First 6 devices that it discovers over MQTT
- Each box should include last 2 digits of mac address

### BLE

Use phone

### Messages

JSON Library


## Extra Credit

When you press a box on the core2, it will toggle that status
