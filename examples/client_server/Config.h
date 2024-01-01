/*
Copyright (c) 2023 Adam Kaniewski

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once


namespace Config {

static const int SERVER_PORT = 6868;
static const std::string SERVER_URL = "localhost";
static const std::string MSG_FOR_SERVER = "Hello Server";
static const std::string MSG_FOR_CLIENT = "Hello Client";

static const std::string AUTH_CERT= R"""(
-----BEGIN CERTIFICATE-----
MIIDUzCCAjugAwIBAgIUDPhUy698d2Ea0e0rPiuuJpgSz04wDQYJKoZIhvcNAQEL
BQAwOTESMBAGA1UEAwwJbG9jYWxob3N0MQswCQYDVQQGEwJVUzEWMBQGA1UEBwwN
U2FuIEZyYW5zaXNjbzAeFw0yMzA5MDExMDQ5MDFaFw0yNDA4MjIxMDQ5MDFaMDkx
EjAQBgNVBAMMCWxvY2FsaG9zdDELMAkGA1UEBhMCVVMxFjAUBgNVBAcMDVNhbiBG
cmFuc2lzY28wggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC+ag4ivzAA
yb7jbeDSfAC/QtSeprTqr2fhWCW8nHa1txSHlayCzi2/q30ljemT1gXsFINkJd3k
y0Q4avKv23JxJuZ0HLoeL5EvY4lll80q/jEgLY4ea5PCJUnD1edVvP+b3PcdpJ9S
TKzM4fStT7fFO3BYoxZ7m6Ib0XbxEP0AEf7dG0Wtrijvm7zjsJVbR5VwiqBhsdZH
g/23i6iDRkiNfGI2hGPvKnZqkXl24QDLLzi3stmE2QW4O5IhFZdUQNg2oXhfQXmf
NY+G3uZXY2MQKxZqRj6+qmnvNDNIsNQHHLYrR8tJiRX5d30OuLqfH6o4FBiuEL80
D3bCkCEtg4+BAgMBAAGjUzBRMB0GA1UdDgQWBBR27Q9lsiDv0bn76PPBFkUsHiE8
2TAfBgNVHSMEGDAWgBR27Q9lsiDv0bn76PPBFkUsHiE82TAPBgNVHRMBAf8EBTAD
AQH/MA0GCSqGSIb3DQEBCwUAA4IBAQBf4IpzDs5gzpBx+npAYzOj0Kvxnm9dHdIL
IFEl4e4qUsJrf3T+h3vw8bWS+tXzC42d5/yLRWKJNMFaT3zF4va5ZD44f3gRZrz2
F9waRtW7mxf/AJk8npL565shOfNxuQWSO2AxCFjxzjvj5oykdcUAPJzb351RKvWf
kduwf2KgwSPHmFpkNHeCvdNdiTa1fRupKetKjG8KADs2ZC2vXpNIxRShn+0q0cLu
TI1qotehv8gvqrpJtnWdLjWqEoOJC/R2WoE7dPU8FVVz3xzEO/WTKlNGrETinCC+
pPCCYuOZQX8vPkM9nYgQ0mdfvpyxBYsZv1o2DDEtvoRN5sbkNiMY
-----END CERTIFICATE-----
)""";

static const std::string SERVER_CERT = R"""(
-----BEGIN CERTIFICATE-----
MIIDvDCCAqSgAwIBAgIUSkunt6zUkmOcjJWQe6Qs33M9GuowDQYJKoZIhvcNAQEL
BQAwOTESMBAGA1UEAwwJbG9jYWxob3N0MQswCQYDVQQGEwJVUzEWMBQGA1UEBwwN
U2FuIEZyYW5zaXNjbzAeFw0yMzA5MDExMDQ5MDFaFw0yNDA4MzExMDQ5MDFaMHEx
CzAJBgNVBAYTAlVTMRMwEQYDVQQIDApDYWxpZm9ybmlhMRYwFAYDVQQHDA1TYW4g
RnJhbnNpc2NvMQwwCgYDVQQKDANBTlkxETAPBgNVBAsMCEFueSBVbml0MRQwEgYD
VQQDDAsqLmxvY2FsaG9zdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB
AItyFSKfS/RAQrJ278xmyDcrA/U7+BriMgINtlU4JF5Il4F0z5/J9n/8J5NFtm+X
kyqA1YR4B4Baj3S0sIFF2VRtt5Tu9lIfsY9r3SBp33t+wwc1AQBRSEvpvz+pquaZ
prdLgHpz5CeDsZZCcmM6FliRhbXKdvArZ3IJVhEBgbpu2zse+IVW+ngikHICYDpT
tGx4Py5ZnNfc6ck9jKUo4MISGoKyLYHogNVQ+ZQ49+o/+eU3Znj0tMfQ9148s7Kf
l4aIXLptmiQ1yyuU+QZSj7q8w/qUcmh4D1P1mS2SbevaNBRfGg1BJ5v/PPiIScWH
Jiy6rQEutisjVrliCJna+wkCAwEAAaOBgzCBgDAfBgNVHSMEGDAWgBR27Q9lsiDv
0bn76PPBFkUsHiE82TAdBgNVHQ4EFgQUWWzYMvuJFyjw6DcsXGZhAOfvr54wCQYD
VR0TBAIwADAOBgNVHQ8BAf8EBAMCBaAwIwYDVR0RBBwwGoIJbG9jYWxob3N0gg13
d3cubG9jYWxob3N0MA0GCSqGSIb3DQEBCwUAA4IBAQAAX1MuA+yFdqJBqbWLE6v3
w/y5A5xtJMU/JPgPMKIYQit3jKgwnoTnqDNIJ37u5bVa0kYLPkEaSyasc1DZaYJR
CC8ejsE1cW8Z3+SkdcxfXT52pP/WZpY0eFIkR79T0L2b7pJEsA194LdNnAgmkQFL
2B88N7hu+sOFkPtk2trtQZTbdIt7pFYKT2Qi2+XXE86ipQ2Tqjb/8ka4wvzKCkCQ
AYbVmAvgnWVoGaXRrIPrBZkvpRkj1cm/1bOdZNJbj+LamAf0UEXS/7JcxZv0W75B
7oNDe9GVXC65njk29VuvilsDgk+gk93SQN7N04lAoSlmo2i183suue6GOjQbhSZJ
-----END CERTIFICATE-----
)""";

static const std::string SERVER_KEY = R"""(
-----BEGIN PRIVATE KEY-----
MIIEvAIBADANBgkqhkiG9w0BAQEFAASCBKYwggSiAgEAAoIBAQCLchUin0v0QEKy
du/MZsg3KwP1O/ga4jICDbZVOCReSJeBdM+fyfZ//CeTRbZvl5MqgNWEeAeAWo90
tLCBRdlUbbeU7vZSH7GPa90gad97fsMHNQEAUUhL6b8/qarmmaa3S4B6c+Qng7GW
QnJjOhZYkYW1ynbwK2dyCVYRAYG6bts7HviFVvp4IpByAmA6U7RseD8uWZzX3OnJ
PYylKODCEhqCsi2B6IDVUPmUOPfqP/nlN2Z49LTH0PdePLOyn5eGiFy6bZokNcsr
lPkGUo+6vMP6lHJoeA9T9Zktkm3r2jQUXxoNQSeb/zz4iEnFhyYsuq0BLrYrI1a5
YgiZ2vsJAgMBAAECggEAL7lgJWTKbHIvwX92K4kDvenRZXVcQOsoPU6vF/RqcgA7
Jfmx81h+LihA7STfPsrr+ZB1QuyHeLTFwyXyAJREwKH6QhUUpV+md9TtLaI+c7gb
DFTmHlMzQJPjueiaDmkIx1OmSYJvU9y9AxWDq5UeP7qyPdjoouLf+QXb+F8BJKo5
p49s4sstHiyfusiCrcGFW0+iu8K9JPO239VOTa4R22P43V/5pyoDljUYXlT4tm4d
08mCBf9BlzhhzDBlzcvq5pLfV/tpxVRv4oNkGhwEJUsLcGZS/MU8LRJ814gtZ63h
QkwgNd23Y3KaiZM7Il9xe7OkdvsrDxFmoXA/cKUqzwKBgQDCwxXH/DMkY6b0pI5q
kWZGwz1z8YWRF9HMnYvpmwoqaAqDW2OJ4ToFzUnRnnANW8S988U5viSCDS50P8PD
yR/v8YLGkHXaj1lwdIg8swWFdfhSXS18ZPVPI7MEyaKdc/boP4fs2vFbEImtd4mu
+fU7DQjyBOD0hb8mou+Z2ViYnwKBgQC3Sm6js5eJTBBkEkBEo3a7YitG1I9q2HnI
F/aqFmnuzbJIYQhyEYfyE+hJjQzgreS1nkh6PqdRzvINjZS8I1h8nyR75beyyjMW
z40tNgSllhS5WAiMqmiNM2KxT+N/9XZMMFx/fnPIcgeqW5+hVX78KxPsxpWarXMk
nPrTaMLDVwKBgF6iNB8sKB1yXllY6+pUJAIWM/H2VRQfIs3tb2Je9x2L2C/jZAZG
7c6L5WZ5ObED+iDiJd7cyRh4Fy5dHV4wh9gq3YRmBR3hsW+278uuoOwsWjCLHeLn
m1qM2htlbiJbiolm9UDdUZESBBPJlZjz03iR6NrQDA5EPpEjcpCqzyW5AoGAO4uv
0/jlUxJmBMy1GldsnEWxP4qQqSp4rduzI+CyO4WR1NfJMZ2mZ6u1/CcZtuhnc+Vf
MG+oVfiARIzpdBGbjqj45lNj0W7FGDUHbVeuPMk8JFaIOIKoAq9d8bK9evWYn/na
IwVGwijheqPXgUdQQMYR7As70Qtma8+f4QyYsXMCgYBy/XSdsOO5B7v2zDTnqViv
/lNI7ICidBfRLATKil2I9F7vIgvrE5WvL4Qe/d0iK6706bqSMJ5hcUQ8uoWsl/5G
882y0bCvlP+QuTaIh8qK3wLbZujDShGRTdv2nVB3XvKacCBjdFghPOZ98cDv7gPm
ARHX5qjnO7IgIMwngcvMeA==
-----END PRIVATE KEY-----
)""";

static const std::string CLIENT_CERT = R"""(
-----BEGIN CERTIFICATE-----
MIIDyTCCArGgAwIBAgIUfvJNKsyfUadg/VqgTxb4Om8nQ9swDQYJKoZIhvcNAQEL
BQAwOTESMBAGA1UEAwwJbG9jYWxob3N0MQswCQYDVQQGEwJVUzEWMBQGA1UEBwwN
U2FuIEZyYW5zaXNjbzAeFw0yMzA5MDExMDQ5MDFaFw0yNDA4MzExMDQ5MDFaMHEx
CzAJBgNVBAYTAlVTMRMwEQYDVQQIDApDYWxpZm9ybmlhMRYwFAYDVQQHDA1TYW4g
RnJhbnNpc2NvMQwwCgYDVQQKDANBTlkxETAPBgNVBAsMCEFueSBVbml0MRQwEgYD
VQQDDAsqLmxvY2FsaG9zdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB
AKPYuvLAwQVyq3hNSin9i3c80x4TYBXLMM4P0r+Qfs39OoHUzQSWl70qMx6CKXjz
ka+4HE7YCsY7V9vSkl83IXo3Zp9fVZ5Frpj8vqW3LVhSCJMcOe2HHeefzBKEJnkQ
UwPsFixNwHmaHm45uwy55eIvu8p6aTkruPNFDVldTJwusvslGEnuSmjkG7KNQ0lv
dyAIP+3V+N5tJRVFUBR1jcV4LKGaPfqt2ezFA6MtG8JqFU3xgNIPE+9N9HnHT2PH
15i4N8FOa+pPhKrqs+KyCRzONlizuVx81tx2Y6JSJAlJ8lrRm34Rg2BXHEMOjl/5
hUmCnjmCTkWSbpV4JMsK4r8CAwEAAaOBkDCBjTAfBgNVHSMEGDAWgBR27Q9lsiDv
0bn76PPBFkUsHiE82TAdBgNVHQ4EFgQUz73VP4ZwJ+tN6BOqZ3AFZMCqiqYwCQYD
VR0TBAIwADAOBgNVHQ8BAf8EBAMCBeAwEQYJYIZIAYb4QgEBBAQDAgWgMB0GA1Ud
JQQWMBQGCCsGAQUFBwMCBggrBgEFBQcDBDANBgkqhkiG9w0BAQsFAAOCAQEAWYf0
X8WU20HVwPqJ7sGjSdvd+rOcCqS8kMYCQvIoR/aKiVxQkTjKPyo4v6LKOszRd/tR
YIxwtW305zGhVOezPycKkAnJfiXf/eQZSL/+pYCl1XkOrIXNIQQ+07UBqjlsVVSn
dSz4cU1Ye+cGY1SJGVky8iI6SqIu3IBasUyYj/M60uMLJ1ysJeEdZsaivHYMGIp4
bsWQxsupro0F7Vd25QKze9wesYnYuECowxp0vVnibNVv9zly8rRpZA0aXtBPjY+y
EpW+1p0VpPXsTfglEMtjE7pAiInpD1g9UpRr8qVBIj5KMKWUSRORam3lpuieP2Eo
HUDAHY79ymchmEnEkg==
-----END CERTIFICATE-----
)""";

static const std::string CLIENT_KEY = R"""(
-----BEGIN PRIVATE KEY-----
MIIEvAIBADANBgkqhkiG9w0BAQEFAASCBKYwggSiAgEAAoIBAQCj2LrywMEFcqt4
TUop/Yt3PNMeE2AVyzDOD9K/kH7N/TqB1M0Elpe9KjMegil485GvuBxO2ArGO1fb
0pJfNyF6N2afX1WeRa6Y/L6lty1YUgiTHDnthx3nn8wShCZ5EFMD7BYsTcB5mh5u
ObsMueXiL7vKemk5K7jzRQ1ZXUycLrL7JRhJ7kpo5BuyjUNJb3cgCD/t1fjebSUV
RVAUdY3FeCyhmj36rdnsxQOjLRvCahVN8YDSDxPvTfR5x09jx9eYuDfBTmvqT4Sq
6rPisgkczjZYs7lcfNbcdmOiUiQJSfJa0Zt+EYNgVxxDDo5f+YVJgp45gk5Fkm6V
eCTLCuK/AgMBAAECggEAD8yUzlau5icWCL4vMhhcS+2mD+bfY4qZ1Kzq7gTxbUyH
SUCrWk9pJ1j73+u+MwcNm8udky667z2GrdBCxAKtbDsOXrzTJTVj7iWk/pbIvRT/
9auJnHFkfVLH21s52H+t+ZlY1AHVVwR4/bj8Y/BDDgn7Sj1+iwA/z9lvPZhFDGQ+
sFFT9S5rvPD4cXFPY6y1co+UBvRGDgJE1w1HOQnwQYkLg/ZKQGim1VCszj0CIh3n
57yt8w0E1NqfFxXwL1F3xNdjLOCiQB1r/495TsKgiv63FCihQqK0WypjDYIbhPKK
FnHVhyGwVnbOFpzY+wJFcZX8igLICkSDc78M8S5IuQKBgQDjXlAAcm0ksE9jAL3n
D8HY+xpWVZxJxPJsn+adfT9jHZVDSBhY+Pfxn/2oU2moV2kLS3Low3OE9Sn9ydXS
e9mkykyK/RBUvMi4zRkNf3lE7N7VSrNgybF0zzcw29zPH7NSLBon8aSarN/+KT2w
HC0gDWmyEkxjpli2w5JB89xG2QKBgQC4eql0BViPYB6znTiM0S4MXBovktNLt9vJ
mezLP4WsxgNl6ooWzhr4Cgn4P7/e9znp4UDE1L22eqN9aeGEJZRtAYmMyzfJP2eP
FSf1+rW0qso/x/4Pe8QnA61A/5H9tqUv/fJAhzYPcTTH9e1SiHOudiZ89GDf8hsV
fNkeMqnnVwKBgCY4r4gztCdLwDyrXkEeg/6+z4Gd6KT6B6q9Ap6M9H/r9DCYSQeD
6uJie9i1FPjooM72Qv3gBWZfdEymhlxY15B9NXrT6c/k4HU4fTKosDEnBJqkcR0f
KsaxIt0CyM7EzHdO1KLMW1hpIUpvkmAtjGRr/M2jh9Xkm9s1azvNC8WZAoGAdAyd
yqGOS2tRBPaTW2o/iIDr7B92EjzP42uUHWuRYNWTz6oSQjRXnM9b5DHeNpnZcbpE
r02RJljRvhQ8TJADZetApzQznMLpF4ZchA25/rQt1J+vv/fAtjZG6AudwDgEhENB
ALzOEjrJ1DxtlazG8OncXmCxDE/WH9dREgEXBrMCgYAzpD5un2ECjKRLLjOFAici
BaM+TwMDJwsHDxrmyi9pvrqma8y/Dw67XFzf0kisUlVoGSOBCVCC1BOBDg5aajrE
S5MWck1cjacpUKD6BZk4ICjltDd4Su/h/S8ra0qHzk7j++IJmyW/MJg7MF9P8+9i
hPmzTW3WaQu0nVCkZJzYxA==
-----END PRIVATE KEY-----
)""";

} // namespace Config