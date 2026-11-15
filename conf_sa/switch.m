%  This script contains an example of how to test criteria for
%  click-free switch in AudioBIFS. Input file supposed 32-bit, mono
%
%      ***** MATLAB Code Starts Here *****


clear;
infile=input('Name of the input file ','s');
fid = fopen(infile,'r');
yy = fread(fid, inf, 'int32');
yy = yy/2147483648;  %convert to -1 1

offset = 56000;   % 7 seconds at 8 kHz
for i=1:8000,
	test(i) = yy(i+offset);
end

trans = fft(test);
abstrans = abs(trans);  % absolute value of the fft

peak = max(abstrans);
abstrans = abstrans/peak; % normalize to 1

abstrans = 20*log10(abstrans);  % values in decibels
plot(abstrans);
hold;
grid;

axis([300 1100 -60 0])
