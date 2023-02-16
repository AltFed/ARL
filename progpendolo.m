clc
clear all
close all
%%
M = 0.07;
m = 0.031;
b = 0.01;
g = 9.8;
l = 0.19;
I = 1/12 * m*l^2;
q = (M)*(I+m*l^2)-(m*l)^2;
s = tf([1 0], 1);
T=0.02;
P= (m*l*s)/(q*((s^3 + (b*(I + m*l^2))*s^2/q - ((M + m)*m*g*l) *s/q - b*m*g*l*s/q)));
P1=c2d(P,T,'zoh');
R=pade(exp(-T*s/2),10);
P2=d2c(P1,'tustin');

figure(1)
nyquist(P2)
grid on 
title("Diagramma di Nyquist del processo nel dominio di Tustin")

figure(2)
bode(P2)
grid on 
title("Diagramma di Bode del processo nel dominio di Tustin")
figure(3)
rlocus(P2)
grid on 
title("Luogo delle radici del processo nel dominio di Tustin")
%% errore nullo a regime riferimento 
K=30;
C0=1/s;
C1=K*C0*(0.1*s+1)^2/(1+s/50)
L=minreal(C1*P);
figure(1)
bode(L)
grid on 

figure(2)
rlocus(L)
grid on
%% rete ancitipatrice 
w=80;
alfa=0.3;
tau=1/(w*sqrt(alfa));
A=(1+tau*s)/(1+tau*alfa*s);
C2=C1*A
L1=minreal(C2*P);
figure(1)
bode(L1)
grid on 

figure(2)
rlocus(L1)
grid on

%% analisi delle prestazioni
T=0.02;
R=pade(exp(-T*s/2),10);
L2=minreal(L1*R);
Wyr=minreal(L2/(1+L2));
figure(5)
bode(L2)
grid on
t=0:20;
figure(1)
step(Wyr,t)
figure(2)
impulse(Wyr,t)

%% discretizzo controllore
[Acs,Bcs,Ccs,Dcs] = ssdata(C2);
C2z=c2d(C2,T,'tustin');
[Ac,Bc,Cc,Dc]=ssdata(C2z)








%%
% N = 1000;
% Xc = zeros(1,N);
% R = ones(1,N);
% e = ones(1,N);
% u = ones(1,N);
% % e(1) = R(1)-yp(1);
% for k = 2:N
%     %Discrettizazione del controllore mediante metodo Tustin
%     Xc(k) = Acd*Xc(k-1) + Bcd*e(k-1);
%     u(k) = Ccs*Xc(k) + Dcs*e(k);
% end



% Wur=minreal((C2)/(1+L2));
% figure(3)
% impulse(Wur,t)
% grid on 
% figure(4)
% step(Wur,t)
% grid on
% figure(5)
% bode(Wur)