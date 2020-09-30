import minimalmodbus

 # port name, slave address (in decimal)
instrument = minimalmodbus.Instrument('COM4', 1, debug=True) 

instrument.serial.baudrate = 115200         # Baud
instrument.serial.bytesize = 8
# instrument.serial.parity   = serial.PARITY_NONE
instrument.serial.stopbits = 1
instrument.serial.timeout  = 0.1          # seconds

## Read temperature (PV = ProcessValue) ##
# Registernumber, number of decimals
temperature = instrument.read_register(289, 1)  
print(temperature)

## Change temperature setpoint (SP) ##
# NEW_TEMPERATURE = 95
# Registernumber, value, number of decimals for storage
# instrument.write_register(24, NEW_TEMPERATURE, 1)  