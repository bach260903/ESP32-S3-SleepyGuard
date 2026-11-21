# CH343 port check during final validation (2026-11-21)

The optional 35-minute US3 capture stopped receiving Serial after 20.9
minutes and 19 successful recoveries. The CH343 still enumerated as COM5
with Windows PnP status OK, but two attempts to reopen it through pyserial
failed with:

`Cannot configure port ... PermissionError(13, 'A device attached to the system is not functioning.', None, 31)`

Windows `pnputil /restart-device` for that CH343 instance reported `Access
is denied`. A later open/close probe produced the same pyserial error.
No FFC cable was touched. T027's clean production build and binary check
passed; T028's flash and 125-second board capture await a working USB-UART
port. The interruption does not by itself establish a firmware failure.
An `idf.py -p COM5 flash` attempt with the production binary also failed
before writing, with the same Windows error 31; its full output is in
`07-final-flash-attempt.txt`. The board therefore still runs the last
fault-injection firmware until CH343 can be reopened and the normal image
is flashed.
