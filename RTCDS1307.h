#ifndef CRTC_H
#define CRTC_H

#include "Wire.h"

#define RTCADDRESS 0x68

class CRTC {
  private:
    uint16_t _offset, _year;
    uint8_t _month, _weekday, _day, _hour, _minute, _second;
    bool _mode, _period;

    uint8_t bcd(uint8_t data) {
      return data + (6 * (data / 10));
    }

    uint8_t decimal(uint8_t data) {
      return data - (6 * (data >> 4));
    }

  public:
    const uint16_t &year = _year;
    const uint8_t &month = _month, &weekday = _weekday, &day = _day, &hour = _hour, &minute = _minute, &second = _second;
    const bool &mode = _mode, &period = _period;

    explicit RTCDS1307(uint16_t offset): _offset(offset) {
      Wire.begin();
    };

    bool midday(bool state) {
      if ((read()) && (state ^ _mode)) {
        if (state) {
          _hour -= (_period = (_hour > 11)) * 12;
          _hour += (_hour == 0) * 12;
        } else _hour = (_hour % 12) + (_period * 12);
        return write(_year, _month, _day, _hour, _minute, _second, state, _period);
      }
      return false;
    }

    bool isLeapYear(uint16_t Y) {
      return !((Y % 4) * (!(Y % 100) + (Y % 400)));
    }

    bool read() {
      Wire.beginTransmission(RTCADDRESS);
      Wire.write(0);
      Wire.endTransmission();

      Wire.requestFrom(RTCADDRESS, 7);

      _second = decimal(Wire.read());
      _minute = decimal(Wire.read());

      uint8_t data = Wire.read();
      _mode = bitRead(data, 6);
      _period = mode & bitRead(data, 5);
      _hour = decimal(data & (_mode ? 0x1F : 0x3F));

      _weekday = decimal(Wire.read());
      _day = decimal(Wire.read());
      _month = decimal(Wire.read());
      _year = decimal(Wire.read()) + _offset;
      return true;
    }

    bool setTimestamp(unsigned long t) {
      uint8_t s = t % 60;
      uint8_t m = (t /= 60) % 60;
      uint8_t h = (t /= 60) % 24;
      bool p = _mode * (h > 11);
      h += (_mode * !h * 12);

      uint16_t Y;
      for (Y = 1970; t > (365 + isLeapYear(Y)); Y++) t -= (365 + isLeapYear(Y));

      uint8_t M;
      uint8_t n[12] = {31, 28 + isLeapYear(Y), 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
      for (M = 1; t >= n[M - 1]; M++) t -= n[M - 1];

      if (Y < _offset) return false;
      else return write(Y, M, t + 1, h, m, s, _mode, p);
    }

    uint8_t wday(uint16_t Y, uint8_t M, uint8_t D) {
      uint8_t n[12] = {31, 28 + isLeapYear(Y), 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
      Y = (D + (((Y - 1) * 365UL) + ((Y - 1) / 4) - ((Y - 1) / 100) + ((Y - 1) / 400))) % 7;
      while (M > 1) Y += n[--M - 1];
      return (Y % 7) + 1;
    }

    bool write(uint16_t Y, uint8_t M, uint8_t D, uint8_t h, uint8_t m, uint8_t s, bool mode = false, bool period = false) {
      if ((s == min(uint8_t(59), s)) && (m == min(uint8_t(59), m)) && (h == constrain(h, mode, uint8_t(mode ? 12 : 23)))) {
        uint8_t n[12] = {31, 28 + isLeapYear(Y), 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

        if ((D == constrain(D, 1, n[M - 1])) && (M == constrain(M, 1, uint8_t(12)))) {
          uint8_t WD = wday(Y, M, D);
          Y -= _offset;

          if (Y == min(uint8_t(99), uint8_t(Y))) {
            _year = Y + _offset;
            Wire.beginTransmission(RTCADDRESS);
            Wire.write(0x00);
            Wire.write(bcd(_second = s));
            Wire.write(bcd(_minute = m));
            Wire.write(bcd(_hour = h) | ((_mode = mode) << 6) | ((mode & (_period = period)) << 5));
            Wire.write(bcd(_weekday = WD));
            Wire.write(bcd(_day = D));
            Wire.write(bcd(_month = M));
            Wire.write(bcd(Y));
            Wire.endTransmission();
            return true;
          }
        }
      }
      return false;
    }
};

#endif