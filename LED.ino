/*License and Copyright
Swfltek Time Library ©2012 Michael Duane Rice swfltek.com
Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in
the documentation and/or other materials provided with the
distribution.
* Neither the name of the copyright holders nor the names of
contributors may be used to endorse or promote products derived
from this software without specific prior written permission.*/


//The following two functions are based on TimeLord library
//http://swfltek.com/arduino/timelord.html
//converted to use unix time.
//must #define LATITUDE and LONGITUDE

//Approximate LATITUDES and LONGITUDES of coral reefs
//Great Barrier Reef
#define LATITUDE -18.2861
#define LONGITUDE 147.7
#define TZONE 10
#define sLATITUDE -65830
#define sLONGITUDE 531720
//Fiji
//#define LATITUDE -18.0
//#define LONGITUDE 179.0
//#define TZONE 12
//Tonga
//#define LATITUDE -21.1333
//#define LONGITUDE -175.2
//#define TZONE 13
//Hawaii
//#define LATITUDE 21.3114
//#define LONGITUDE -157.7964
//#define TZONE -10
//Carribean
//#define LATITUDE 14.5256
//#define LONGITUDE -75.8183
//#define TZONE -4
//Bahamas
//#define LATITUDE 25.0667
//#define LONGITUDE -77.3333
//#define TZONE -5
//Red Sea
//#define LATITUDE 22.0000
//#define LONGITUDE 38.0
//#define TZONE 3
//Philippines
//#define LATITUDE 10.0
//#define LONGITUDE 118.8333
//#define TZONE 8

#define TROP_YEAR 31556925
#define ANOM_YEAR 31558433
#define INCLINATION 0.409105176667471    /* Earths axial tilt at the epoch */
#define PERIHELION 31316400    /* perihelion of 1999, 03 jan 13:00 UTC */
#define SOLSTICE 836160        /* winter solstice of 1999, 22 Dec 07:44 UTC */
#define TWO_PI 6.283185307179586
#define TROP_CYCLE 5022440.6025
#define ANOM_CYCLE 5022680.6082
#define DELTA_V 0.03342044    /* 2x orbital eccentricity */
#define LAG 38520

//set to value from 12 to 16
#define LEDPWMRESOLUTION  12

boolean initLED(){
  //D2 PE4, D3 PE5, D5 PE3, D6 PH3, D7 PH4, D8 PH5
  DDRE |= _BV(PE3) | _BV(PE4) | _BV(PE5);
  DDRH |= _BV(PH3) | _BV(PH4) | _BV(PH5);
  TCCR3A |=  _BV(WGM31) | _BV(COM3A1) | _BV(COM3B1) | _BV(COM3C1);
  TCCR3A &= ~(_BV(WGM30) | _BV(COM3A0) | _BV(COM3B0) | _BV(COM3C0));
  TCCR3B |= _BV(WGM33) | _BV(WGM32) | _BV(CS30);
  TCCR3B &= ~(_BV(CS32) | _BV(CS31));
  TCCR4A |=  _BV(WGM41) | _BV(COM4A1) | _BV(COM4B1) | _BV(COM4C1);
  TCCR4A &= ~(_BV(WGM40) | _BV(COM4A0) | _BV(COM4B0) | _BV(COM4C0));
  TCCR4B |= _BV(WGM43) | _BV(WGM42) | _BV(CS40);
  TCCR4B &= ~(_BV(CS42) | _BV(CS41));
  ICR3 = 2^LEDPWMRESOLUTION-1;//12 bit resolution
  ICR4 = 2^LEDPWMRESOLUTION-1;
  OCR3A = 0;//initially 0ff
  OCR3B = 0;
  OCR3C = 0;
  OCR4A = 0;
  OCR4B = 0;
  OCR4C = 0;
}

time_t sunrise(time_t t){
  return computeSun(t,true);
}

time_t sunset(time_t t){
  return computeSun(t,false);
}

time_t computeSun(time_t t, bool isSunrise) {
  uint8_t  month, day;
  float y, decl, eqt, ha, lon, lat, z;
  uint8_t a;
  int minutes;
  
  tmElements_t tm;
  breakTime(t,tm);
  month=tm.Month-1;
  day=tm.Day-1;
  lon=-LONGITUDE/57.295779513082322;
  lat=LATITUDE/57.295779513082322;
  //approximate hour;
  a=6;
  if(isSunrise) a=18;
  // approximate day of year
  y= month * 30.4375 + day  + a/24.0; // 0... 365
  // compute fractional year
  y *= 1.718771839885e-02; // 0... 1
  // compute equation of time... .43068174
  eqt=229.18 * (0.000075+0.001868*cos(y)  -0.032077*sin(y) -0.014615*cos(y*2) -0.040849*sin(y* 2) );
  // compute solar declination... -0.398272
  decl=0.006918-0.399912*cos(y)+0.070257*sin(y)-0.006758*cos(y*2)+0.000907*sin(y*2)-0.002697*cos(y*3)+0.00148*sin(y*3);
  //compute hour angle
  ha=(  cos(1.585340737228125) / (cos(lat)*cos(decl)) -tan(lat) * tan(decl)  );
  if(fabs(ha)>1.0){// we're in the (ant)arctic and there is no rise(or set) today!
  	return 0; 
  }
  ha=acos(ha); 
  if(isSunrise==false) ha=-ha;
  // compute minutes from midnight
  minutes=720+4*(lon-ha)*57.295779513082322-eqt;
  // convert from UTC back to our timezone
  minutes+= TZONE*60;
  // adjust the time array by minutes
  tm.Hour=0;
  tm.Minute=0;
  tm.Second=0;
  return makeTime(tm)+minutes*SECS_PER_MIN;
}


float moonPhase(time_t t){
// the period is 29.530588853 days
// we compute the number of days since Jan 6, 2000
// at which time the moon was 'new'
  time_t newmoon = 947116800;
  long d;
  float p;

  d = elapsedDays(t) - elapsedDays(newmoon);
  p=d/29.530588853; // total lunar cycles since 1/1/2000
  d=p;
  p-=d; // p is now the fractional cycle, 0 to (less than) 1
  return p;
}


int
equation_of_time(const time_t * timer)
{
	int32_t         s, p;
	float          pf, sf, dV;

	/* compute orbital position relative to perihelion */
	p = *timer % ANOM_YEAR;
	p += PERIHELION;
	pf = p;
	pf /= ANOM_CYCLE;
	pf = sin(pf);

	/* Derive a velocity correction factor from the perihelion angle */
	dV = pf * DELTA_V;

	/* compute approximate position relative to solstice */
	s = *timer % TROP_YEAR;
	s += SOLSTICE;
	s *= 2;
	sf = s;
	sf /= TROP_CYCLE;

	/* modulate to derive actual position */
	sf += dV;
	sf = sin(sf);

	/* compute contributions */
	sf *= 592.2;
	pf *= 459.6;
	s = pf + sf;
	return -s;

}

time_t
solar_noon(const time_t * timer)
{
	time_t          t;
	long            n;

	/* determine time of solar noon at the prime meridian */
	t = *timer % SECS_PER_DAY;
	t = *timer - t;
	t += 43200L;
	t -= equation_of_time(timer);

	/* rotate to observers longitude */
	n = sLONGITUDE / 15L;
	t -= n;

	return t;

}

float
solar_declination(const time_t * timer)
{

	uint32_t        fT, oV;
	float          dV, dT;

	/* Determine orbital angle relative to perihelion of January 1999 */
	oV = *timer % ANOM_YEAR;
	oV += PERIHELION;
	dV = oV;
	dV /= ANOM_CYCLE;

	/* Derive velocity correction factor from the perihelion angle */
	dV = sin(dV);
	dV *= DELTA_V;

	/* Determine orbital angle relative to solstice of December 1999 */
	fT = *timer % TROP_YEAR;
	fT += SOLSTICE + LAG;
	dT = fT;
	dT /= TROP_CYCLE;
	dT += dV;

	/* Finally having the solstice angle, we can compute the declination */
	dT = cos(dT) * INCLINATION;

	return -dT;
}

long
daylight_seconds(const time_t * timer)
{
	float          l, d;
	long            n;

	/* convert latitude to radians */
	l = sLATITUDE / 206264.806;

	d = -solar_declination(timer);

	/* partial 'Sunrise Equation' */
	d = tan(l) * tan(d);

	/* magnitude of d may exceed 1.0 at near solstices */
	if (d > 1.0)
		d = 1.0;

	if (d < -1.0)
		d = -1.0;

	/* derive hour angle */
	d = acos(d);

	/* but for atmospheric refraction, this would be d /= M_PI */
	d /= 3.112505;

	n = SECS_PER_DAY * d;

	return n;
}

time_t
sun_set(const time_t * timer)
{
	long            n;
	time_t          t;

	/* sunset is 1/2 'day' after solar noon */
	t = solar_noon(timer);
	n = daylight_seconds(timer) / 2L;
	t += n;

	return t;

}

time_t
sun_rise(const time_t * timer)
{
	long            n;
	time_t          t;

	/* sunrise is 1/2 'day' before solar noon */
	t = solar_noon(timer);
	n = daylight_seconds(timer) / 2L;
	t -= n;

	return t;
}

