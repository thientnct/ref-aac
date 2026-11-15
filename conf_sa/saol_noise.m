%  This script contains an example of how to calculate parameters for
%  conformance testing of the noise generators of SA
%  It is supposed that the decoder output is in a file .wav called noise
%
%      ***** MATLAB Code Starts Here *****

clear;
res=wavread('noise.wav');

index = 0; 
% at 8 kHz read 5 seconds of audio
linear = res(1:40000);

% at 8 kHz read seconds 6-to-11 of audio
linramped = res(48001:88000).^2;
for i=2:2:40000,
   linramped(i) = -linramped(i);
end

% at 8 kHz read seconds 12-to-17 of audio
expon = res(96001:136000);

% at 8 kHz read seconds 18-to-23 of audio
poiss = res(144001:184000);

% at 8 kHz read seconds 24-to-29 of audio
gauss = res(192001:232000);

mean_linear = mean(linear)
var_linear = var(linear)

mean_linear_ramped = mean(linramped)
var_linear_ramped = var(linramped)

mean_expon = mean(expon)

mean_poiss = mean(poiss)

mean_gauusian = mean(gauss)
var_gaussian = var(gauss)