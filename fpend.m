function dy = fpend(t,y)

global link_1 link_2 m_1 m_2 g

dy = zeros(4,1);
dy(1) = y(2); % d(theta1)/dt = omega1
dy(3) = y(4); % d(theta2)/dt = omega2
a = (m_1 + m_2)*link_1;
b = m_2*link_2*cos(y(1)-y(3)); %cos(theta1 - theta2);
c = m_2*link_1*cos(y(1)-y(3)); 
d = m_2 * link_2;
e = -m_2 * link_2 * y(4) *  y(4) * sin(y(1)-y(3)) - g * (m_1+m_2) * sin (y(1));
f = m_2 * link_1 * y(2) *  y(2) * sin(y(1)-y(3)) -m_2*g*sin(y(3));

dy(2) = (e*d-b*f)/(a*d-c*b); %d(omega1)/dt = alfa1
dy(4) = (a*f-c*e)/(a*d-c*b); %d(omega2)/dt = alfa2


end