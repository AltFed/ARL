clc 
clear all
close all
%% PARAMETRI INIZIALI
global link_1 link_2 m_1 m_2 g
link_1 = 1;
link_2 = 2;
m_1 = 1;
m_2 = 1.5;
g = 9.8;

%% CONDIZIONI INIZIALI
Time = 70;
theta_1 = 1.6;
omega_1 = 0;
theta_2 = 2.2;
omega_2 = 0;

y0 = [theta_1 omega_1 theta_2 omega_2];
[t,y] = ode45(@fpend, [0,Time],[1.6 0 2.2 0]);
[t,y_1] = ode45(@fpend, [0,Time],[1.601 0 2.201 0]);
%% POSIZIONI
x1=link_1*sin(y(:,1));
y1=-link_1*cos(y(:,1));
x2=link_1*sin(y(:,1))+link_2*sin(y(:,3));
y2=-link_1*cos(y(:,1))-link_2*cos(y(:,3));

x1_1=link_1*sin(y_1(:,1));
y1_1=-link_1*cos(y_1(:,1));
x2_1=link_1*sin(y_1(:,1))+link_2*sin(y_1(:,3));
y2_1=-link_1*cos(y_1(:,1))-link_2*cos(y_1(:,3));
%% GRAFICO

figure(1)
   plot(x1,y1,'linewidth',2)
   hold on
   plot(x2,y2,'r','linewidth',2)
   h=gca; 
   get(h,'fontSize') 
   set(h,'fontSize',14)
   xlabel('X','fontSize',14);
   ylabel('Y','fontSize',14);
   title('Chaotic Double Pendulum','fontsize',14)
   fh = figure(1);
   set(fh, 'color', 'white'); 
   hold on 

   figure(2)
   plot(x1_1,y1_1,'linewidth',2)
   hold on
   plot(x2_1,y2_1,'green','linewidth',2)
   h=gca; 
   get(h,'fontSize') 
   set(h,'fontSize',14)
   xlabel('X','fontSize',14);
   ylabel('Y','fontSize',14);
   title('Chaotic Double Pendulum','fontsize',14)
   fh = figure(1);
   set(fh, 'color', 'white'); 
   hold on 
  
 %% TEST FILMATO
 figure(3)
 Ncount=0;
   fram=0;
 v = VideoWriter('peaks.avi');
 open(v); 
     for i=1:length(y)
         Ncount=Ncount+1;
         fram=fram+1;
         plot(0, 0,'.','markersize',20);
         hold on
        
         plot(x1(i),y1(i),'g.','markersize',20);
         plot(x2(i),y2(i),'r.','markersize',20);
         plot(x1_1(i),y1_1(i),'y.','markersize',20);
         plot(x2_1(i),y2_1(i),'b.','markersize',20);
        % drawing lines

         %plot(x1(1:i),y1(1:i),'g','markersize',15);
         plot(x2(1:i),y2(1:i),'r','LineWidth',1.6);
         %plot(x1_1(1:i),y1_1(1:i),'y','markersize',15);
         plot(x2_1(1:i),y2_1(1:i),'b','LineWidth',1.6);

         hold off
         line([0 x1(i)], [0 y1(i)],'Linewidth',2);
         line([0 x1_1(i)], [0 y1_1(i)],'Linewidth',2);
         axis([-(link_1+link_2) link_1+link_2 -(link_1+link_2) link_1+link_2]);
         line([x1(i) x2(i)], [y1(i) y2(i)],'linewidth',2);
         line([x1_1(i) x2_1(i)], [y1_1(i) y2_1(i)],'linewidth',2);
         h=gca; 
         get(h,'fontSize') 
         set(h,'fontSize',12)
         xlabel('X','fontSize',12);
         ylabel('Y','fontSize',12);
         title('Chaotic Motion','fontsize',14)
         fh = figure(3);
         set(fh, 'color', 'white'); 
         frame=getframe;
         writeVideo(v,frame);
         end





