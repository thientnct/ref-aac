%  This script contains an example of how to calculate SNR for SR conversion
%  It is a simple SNR calculation over a second of typical MPEG-4 CUs
%  It is supposed that the reference output is sine_16.wav
%
%      ***** MATLAB Code Starts Here *****

of=input('Offset in samples ? ','s');
offset=str2num(of);

res=wavread('resample.wav');
org=wavread('sine_16.wav');

index = 1;
for i=96000:112000,   % at 16 kHz read the 7th second of audio
	err(index) = org(i) - res(i+offset);
	index = index+1;
end

sqerr = err.^2;
noisep = mean(sqerr); % calculate E[e^2]

sqorg = org.^2;
signp = mean(sqorg);  % calculate E[x^2]

ratio = signp/noisep;
snr = 10*log10(ratio) % calculate snr ad display it
