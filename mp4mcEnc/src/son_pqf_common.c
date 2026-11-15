/*******************************************************************
SC 29 Software Copyright Licencing Disclaimer:

This software module was originally developed by
  Yoshiaki Oikawa (Sony Corporation) and Mitsuyuki Hatanaka (Sony Corporation)

and edited by
  -

in the course of development of the ISO/IEC 13818-7 and ISO/IEC 14496-3 
standards for reference purposes and its performance may not have been 
optimized. This software module is an implementation of one or more tools as 
specified by the ISO/IEC 13818-7 and ISO/IEC 14496-3 standards.
ISO/IEC gives users free license to this software module or modifications 
thereof for use in products claiming conformance to audiovisual and 
image-coding related ITU Recommendations and/or ISO/IEC International 
Standards. ISO/IEC gives users the same free license to this software module or 
modifications thereof for research purposes and further ISO/IEC standardisation.
Those intending to use this software module in products are advised that its 
use may infringe existing patents. ISO/IEC have no liability for use of this 
software module or modifications thereof. Copyright is not released for 
products that do not conform to audiovisual and image-coding related ITU 
Recommendations and/or ISO/IEC International Standards.
The original developer retains full right to modify and use the code for its 
own purpose, assign or donate the code to a third party and to inhibit third 
parties from using the code for products that do not conform to audiovisual and 
image-coding related ITU Recommendations and/or ISO/IEC International Standards.
This copyright notice must be included in all copies or derivative works.
Copyright (c) ISO/IEC 1996.
*******************************************************************/
#include	"sony_local.h"
#include	<math.h>

#define	SCALAR	double
#ifndef PI
#define	PI	(3.14159265358979)
#endif


/*******************************************************************************
	Set Prototype Filter of PQF

	p_proto[96] should have been secured.
*******************************************************************************/
void	son_set_protopqf(
	SCALAR	*p_proto
	)
{
		int	j;
	static	SCALAR	a_half[48] =
{
   1.2206911375946939E-05,  1.7261986723798209E-05,  1.2300093657077942E-05,
  -1.0833943097791965E-05, -5.7772498639901686E-05, -1.2764767618947719E-04,
  -2.0965186675013334E-04, -2.8166673689263850E-04, -3.1234860429017460E-04,
  -2.6738519958452353E-04, -1.1949424681824722E-04,  1.3965139412648678E-04,
   4.8864136409185725E-04,  8.7044629275148344E-04,  1.1949430269934793E-03,
   1.3519708175026700E-03,  1.2346314373964412E-03,  7.6953209114159191E-04,
  -5.2242432579537141E-05, -1.1516092887213454E-03, -2.3538469841711277E-03,
  -3.4033123072127277E-03, -4.0028551071986133E-03, -3.8745415659693259E-03,
  -2.8321073426874310E-03, -8.5038892323704195E-04,  1.8856751185350931E-03,
   4.9688741735340923E-03,  7.8056704536795926E-03,  9.7027909685901654E-03,
   9.9960423120166159E-03,  8.2019366335594487E-03,  4.1642072876103365E-03,
  -1.8364453822737758E-03, -9.0384863094167686E-03, -1.6241528177129844E-02,
  -2.1939551286300665E-02, -2.4533179947088161E-02, -2.2591663337768787E-02,
  -1.5122066420044672E-02, -1.7971713448186293E-03,  1.6903413428575379E-02,
   3.9672315874127042E-02,  6.4487527248102796E-02,  8.8850025474701726E-02,
   0.1101132906105560    ,  0.1258540205143761    ,  0.1342239368467012    
};

	for (j = 0; j < 48; ++j) {
		p_proto[j] = p_proto[95-j] = a_half[j];
	}
}
