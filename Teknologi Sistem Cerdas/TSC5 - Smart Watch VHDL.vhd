library IEEE;
use IEEE.numeric_bit.all;

use IEEE.STD_LOGIC_1164.ALL;
use IEEE.std_logic_unsigned.all;
entity wristwatch is
port(B1, B2, B3, main_clk: in bit;

anode : out STD_LOGIC_VECTOR (7 downto 0);
ledout1 : out STD_LOGIC_VECTOR (6 downto 0);
ring_alarm ,alarm_set_disp: out bit
);
end wristwatch;

architecture wristwatch1 of wristwatch is
	type st_type is (time_state, set_min, set_hours, alarm, set_alarm_hrs,
	set_alarm_min, stop_watch);
	signal state, nextstate: st_type;
	signal inch, incm, alarm_off, set_alarm, incha, incma,
	start_stop, reset: bit;
	signal disp1_input: unsigned(31 downto 0);
	signal counter : integer:=0;
	signal am_pm, aam_pm,ring, alarm_set:  bit;
	signal hours, ahours, minutes, aminutes, seconds:  unsigned(7 downto 0);
	signal swhundreths, swseconds, swminutes:  unsigned(7 downto 0);
	signal clk :  bit;
	signal b1p,b2p,b3p :bit;

component clock is
	port(clk, inch, incm, incha, incma, set_alarm, alarm_off: in bit;
	hours, ahours, minutes, aminutes, seconds: inout unsigned(7 downto 0);
	am_pm, aam_pm, ring, alarm_set: inout bit);
end component;
component stopwatch is
	port(clk, reset, start_stop: in bit;
	swhundreths, swseconds, swminutes: inout unsigned(7 downto 0));
end component;

component seven is
port(main_clk,reset: in bit;
		displayed_number: in unsigned (31 downto 0);
		Anode_Activate : out STD_LOGIC_VECTOR (7 downto 0);
      LED_out : out STD_LOGIC_VECTOR (6 downto 0));
end component;


begin

	clock_map: clock port map(clk, inch, incm, incha, incma, set_alarm, alarm_off,
	hours, ahours, minutes, aminutes, seconds, am_pm,
	aam_pm, ring, alarm_set);
	stopwatch_map: stopwatch port map(clk, reset, start_stop, swhundreths,
	swseconds, swminutes);
	disp1 : seven port map(main_clk,reset,disp1_input,anode,ledout1);
		process(state, B1, B2, B3)
		begin
		alarm_off <= '0'; inch <= '0'; incm <= '0'; set_alarm <= '0'; incha <= '0';
		incma <= '0'; start_stop <= '0'; reset <= '0';
		case state is
		when time_state =>
			if (B1 = '1' and b1p = '0') then nextstate <= alarm;
			elsif (B2 = '1' and b2p = '0') then nextstate <= set_hours;
			else nextstate <= time_state;
			end if;
			if (B3 = '1' and b3p= '0') then alarm_off <= '1';
			end if;
			if am_pm = '0' then
				disp1_input <= "11011111" & hours & minutes & seconds;
			else		
				disp1_input <= "11101111" & hours & minutes & seconds;
			end if;
			
			
		when set_hours =>
			if B3 = '1'  and b3p ='0' then inch <= '1'; nextstate <= set_hours;
			else nextstate <= set_hours;			
			end if;
			if B2 = '1' and b2p = '0' then nextstate <= set_min;
			end if;
			if am_pm = '0' then
				disp1_input <= "11011111" & hours & minutes & seconds;
			else		
				disp1_input <= "11101111" & hours & minutes & seconds;
			end if;
			
		when set_min =>
			if B3 = '1' and b3p = '0' then incm <= '1'; nextstate <= set_min;
			else nextstate <= set_min;
			end if;
			if B2 = '1' and b2p = '0' then nextstate <= time_state;
			end if;
			if am_pm = '0' then
				disp1_input <= "11011111" & hours & minutes & seconds;
			else		
				disp1_input <= "11101111" & hours & minutes & seconds;
			end if;
			
		when alarm =>
			if B1 = '1' and b1p = '0' then nextstate <= stop_watch;
			elsif B2 = '1' and b2p = '0' then nextstate <= set_alarm_hrs;
			else nextstate <= alarm;		
			end if;
			if B3 = '1' and b3p = '0' then set_alarm <= '1'; nextstate <= alarm;
			end if;
			if aam_pm = '0' then
				disp1_input <= "1101111111111111" & ahours & aminutes;
			else		
				disp1_input <= "1110111111111111" & ahours & aminutes;
			end if;
			
		
		when set_alarm_hrs =>
			if B2 = '1' and b2p ='0' then nextstate <= set_alarm_min;
			else nextstate <= set_alarm_hrs;
			end if;
			if B3 = '1'  and b3p = '0' then incha <= '1';
			end if;
			if aam_pm = '0' then
				disp1_input <= "1101111111111111" & ahours & aminutes ;
			else		
				disp1_input <= "1110111111111111" & ahours & aminutes ;
			end if;
			
		when set_alarm_min =>
			if B2 = '1' and b2p = '0' then nextstate <= alarm;
			else nextstate <= set_alarm_min;
			end if;
			if B3 = '1' and b3p = '0' then incma <= '1';
			end if;
			if aam_pm = '0' then
				disp1_input <= "1101111111111111" & ahours & aminutes ;
			else		
				disp1_input <= "1110111111111111" & ahours & aminutes ;
			end if;
		
		
		when stop_watch =>
			if B1 = '1' and b1p = '0' then nextstate <= time_state;
			else nextstate <= stop_watch;
			end if;
			if B2 = '1' and b2p = '0' then start_stop <= '1';
			end if;
			if B3 = '1' and b3p = '0' then reset <= '1';
			end if;
			disp1_input <= "11111111" & swminutes & swseconds & swhundreths;
			
		end case;
		end process;
		process(clk)
	begin
	if (rising_edge(clk)) then -- state clk
	state <= nextstate;
	ring_alarm <= ring;
	alarm_set_disp <= alarm_set;
	b1p <= B1;
	b2p <= B2;
	b3p <= B3;
	end if;
	end process;
	process (main_clk)
	begin 
		if(rising_edge(main_clk)) then
			if( counter >= 999999) then
				clk <= '1';
				counter <= 0;
			else
				counter <= counter + 1;
				clk <= '0';
		   end if;
			end if;
	end process;

end wristwatch1;
