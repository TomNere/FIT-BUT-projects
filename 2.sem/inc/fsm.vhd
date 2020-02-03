-- fsm.vhd: Finite State Machine
-- Author(s): xnerec00, Tomas Nereca
--
library ieee;
use ieee.std_logic_1164.all;
-- ----------------------------------------------------------------------------
--                        Entity declaration
-- ----------------------------------------------------------------------------
entity fsm is
port(
   CLK         : in  std_logic;
   RESET       : in  std_logic;

   -- Input signals
   KEY         : in  std_logic_vector(15 downto 0);
   CNT_OF      : in  std_logic;

   -- Output signals
   FSM_CNT_CE  : out std_logic;
   FSM_MX_MEM  : out std_logic;
   FSM_MX_LCD  : out std_logic;
   FSM_LCD_WR  : out std_logic;
   FSM_LCD_CLR : out std_logic
);
end entity fsm;

-- ----------------------------------------------------------------------------
--                      Architecture declaration
-- ----------------------------------------------------------------------------
architecture behavioral of fsm is
    type t_state is (ENTRY1, ENTRY2A,ENTRY2B, ENTRY3A, ENTRY3B, ENTRY4A, ENTRY4B, ENTRY5A, ENTRY5B, 
                        ENTRY6A, ENTRY6B, ENTRY7A, ENTRY7B, ENTRY8A, ENTRY8B, ENTRY9A, ENTRY9B, ENTRY10A, 
                        ENTRY10B, ENTRY11, VALID_CODE, INVALID_CODE, INVALID_WAIT, FINISH);

    signal present_state, next_state : t_state;

begin
-- -------------------------------------------------------
sync_logic : process(RESET, CLK)
begin
    if (RESET = '1') then
        present_state <= ENTRY1;
    elsif (CLK'event AND CLK = '1') then
        present_state <= next_state;
    end if;
end process sync_logic;

-- -------------------------------------------------------
next_state_logic : process(present_state, KEY, CNT_OF)
begin
    case (present_state) is

-----------------------DECIDE-----------------------------
    when ENTRY1 =>
        next_state <= ENTRY1;
	    if (KEY(1) = '1') then
		    next_state <= ENTRY2A;
        elsif (KEY(2) = '1') then
		    next_state <= ENTRY2B;
        elsif (KEY(15) = '1') then
            next_state <= INVALID_CODE; 
	    elsif (KEY(14 downto 0) /= "000000000000000") then
		    next_state <= INVALID_WAIT;
        end if;

--------------------------CODE1----------------------------
    when ENTRY2A =>
        next_state <= ENTRY2A;
        if (KEY(3) = '1') then
		    next_state <= ENTRY3A;
        elsif (KEY(15) = '1') then
            next_state <= INVALID_CODE; 
	    elsif (KEY(14 downto 0) /= "000000000000000") then
		    next_state <= INVALID_WAIT;
        end if;
	    -- - - - - - - - - - - - - - - - - - - - - - -
    when ENTRY3A =>
        next_state <= ENTRY3A;
        if (KEY(8) = '1') then
		    next_state <= ENTRY4A;
        elsif (KEY(15) = '1') then
            next_state <= INVALID_CODE; 
	    elsif (KEY(14 downto 0) /= "000000000000000") then
		    next_state <= INVALID_WAIT;
        end if;
	    -- - - - - - - - - - - - - - - - - - - - - - -
    when ENTRY4A =>
        next_state <= ENTRY4A;
        if (KEY(0) = '1') then
		    next_state <= ENTRY5A;
        elsif (KEY(15) = '1') then
            next_state <= INVALID_CODE; 
	    elsif (KEY(14 downto 0) /= "000000000000000") then
		    next_state <= INVALID_WAIT;
        end if;
	    -- - - - - - - - - - - - - - - - - - - - - - -
    when ENTRY5A =>
        next_state <= ENTRY5A;
        if (KEY(1) = '1') then
		    next_state <= ENTRY6A;
        elsif (KEY(15) = '1') then
            next_state <= INVALID_CODE; 
	    elsif (KEY(14 downto 0) /= "000000000000000") then
		    next_state <= INVALID_WAIT;
        end if;
	    -- - - - - - - - - - - - - - - - - - - - - - -
    when ENTRY6A =>
        next_state <= ENTRY6A;
        if (KEY(1) = '1') then
		    next_state <= ENTRY7A;
        elsif (KEY(15) = '1') then
            next_state <= INVALID_CODE; 
	    elsif (KEY(14 downto 0) /= "000000000000000") then
		    next_state <= INVALID_WAIT;
        end if;
	    -- - - - - - - - - - - - - - - - - - - - - - -
    when ENTRY7A =>
        next_state <= ENTRY7A;
        if (KEY(3) = '1') then
		    next_state <= ENTRY8A;
        elsif (KEY(15) = '1') then
            next_state <= INVALID_CODE; 
	    elsif (KEY(14 downto 0) /= "000000000000000") then
		  next_state <= INVALID_WAIT;
        end if;
	    -- - - - - - - - - - - - - - - - - - - - - - -
    when ENTRY8A =>
        next_state <= ENTRY8A;
        if (KEY(0) = '1') then
		    next_state <= ENTRY9A;
        elsif (KEY(15) = '1') then
            next_state <= INVALID_CODE; 
	    elsif (KEY(14 downto 0) /= "000000000000000") then
		    next_state <= INVALID_WAIT;
        end if;
	    -- - - - - - - - - - - - - - - - - - - - - - -
    when ENTRY9A =>
        next_state <= ENTRY9A;
        if (KEY(9) = '1') then
		    next_state <= ENTRY10A;
        elsif (KEY(15) = '1') then
            next_state <= INVALID_CODE; 
	    elsif (KEY(14 downto 0) /= "000000000000000") then
		    next_state <= INVALID_WAIT;
        end if;
	    -- - - - - - - - - - - - - - - - - - - - - - -
    when ENTRY10A =>
        next_state <= ENTRY10A;
        if (KEY(7) = '1') then
		    next_state <= ENTRY11;
        elsif (KEY(15) = '1') then
            next_state <= INVALID_CODE;  
	    elsif (KEY(14 downto 0) /= "000000000000000") then
		    next_state <= INVALID_WAIT;
        end if;

--------------------------CODE2-------------------------------
when ENTRY2B =>
        next_state <= ENTRY2B;
        if (KEY(3) = '1') then
		    next_state <= ENTRY3B;
        elsif (KEY(15) = '1') then
            next_state <= INVALID_CODE;
	    elsif (KEY(14 downto 0) /= "000000000000000") then
		    next_state <= INVALID_WAIT;
        end if;
	    -- - - - - - - - - - - - - - - - - - - - - - -
    when ENTRY3B =>
        next_state <= ENTRY3B;
        if (KEY(0) = '1') then
		    next_state <= ENTRY4B;
        elsif (KEY(15) = '1') then
            next_state <= INVALID_CODE; 
	    elsif (KEY(14 downto 0) /= "000000000000000") then
		    next_state <= INVALID_WAIT;
        end if;
	    -- - - - - - - - - - - - - - - - - - - - - - -
    when ENTRY4B =>
        next_state <= ENTRY4B;
        if (KEY(0) = '1') then
		    next_state <= ENTRY5B;
        elsif (KEY(15) = '1') then
            next_state <= INVALID_CODE; 
	    elsif (KEY(14 downto 0) /= "000000000000000") then
		    next_state <= INVALID_WAIT;
        end if;
	    -- - - - - - - - - - - - - - - - - - - - - - -
    when ENTRY5B =>
        next_state <= ENTRY5B;
        if (KEY(1) = '1') then
		    next_state <= ENTRY6B;
        elsif (KEY(15) = '1') then
            next_state <= INVALID_CODE; 
	    elsif (KEY(14 downto 0) /= "000000000000000") then
		    next_state <= INVALID_WAIT;
        end if;
	    -- - - - - - - - - - - - - - - - - - - - - - -
    when ENTRY6B =>
        next_state <= ENTRY6B;
        if (KEY(8) = '1') then
		    next_state <= ENTRY7B;
        elsif (KEY(15) = '1') then
            next_state <= INVALID_CODE; 
	    elsif (KEY(14 downto 0) /= "000000000000000") then
		    next_state <= INVALID_WAIT;
        end if;
	    -- - - - - - - - - - - - - - - - - - - - - - -
    when ENTRY7B =>
        next_state <= ENTRY7B;
        if (KEY(8) = '1') then
		    next_state <= ENTRY8B;
        elsif (KEY(15) = '1') then
            next_state <= INVALID_CODE; 
	    elsif (KEY(14 downto 0) /= "000000000000000") then
		  next_state <= INVALID_WAIT;
        end if;
	    -- - - - - - - - - - - - - - - - - - - - - - -
    when ENTRY8B =>
        next_state <= ENTRY8B;
        if (KEY(4) = '1') then
		  next_state <= ENTRY9B;
        elsif (KEY(15) = '1') then
            next_state <= INVALID_CODE; 
	    elsif (KEY(14 downto 0) /= "000000000000000") then
		    next_state <= INVALID_WAIT;
        end if;
	    -- - - - - - - - - - - - - - - - - - - - - - -
    when ENTRY9B =>
        next_state <= ENTRY9B;
        if (KEY(9) = '1') then
		    next_state <= ENTRY10B;
        elsif (KEY(15) = '1') then
            next_state <= INVALID_CODE;     
	    elsif (KEY(14 downto 0) /= "000000000000000") then
		    next_state <= INVALID_WAIT;
        end if;
	    -- - - - - - - - - - - - - - - - - - - - - - -
    when ENTRY10B =>
        next_state <= ENTRY10B;
        if (KEY(6) = '1') then
		    next_state <= ENTRY11;
        elsif (KEY(15) = '1') then
            next_state <= INVALID_CODE; 
	    elsif (KEY(14 downto 0) /= "000000000000000") then
		    next_state <= INVALID_WAIT;
        end if;

--------------------------END---------------------------------
    when ENTRY11 =>
        next_state <= ENTRY11;
        if (KEY(15) = '1') then
            next_state <= VALID_CODE; 
	    elsif (KEY(14 downto 0) /= "000000000000000") then
		    next_state <= INVALID_WAIT;
        end if;
    -- - - - - - - - - - - - - - - - - - - - - - -
    when VALID_CODE =>
        next_state <= VALID_CODE;
        if (CNT_OF = '1') then
            next_state <= FINISH;
        end if;
    -- - - - - - - - - - - - - - - - - - - - - - -
    when INVALID_CODE =>
        next_state <= INVALID_CODE;
        if (CNT_OF = '1') then
            next_state <= FINISH;
        end if;
    -- - - - - - - - - - - - - - - - - - - - - - -
    when INVALID_WAIT =>
        next_state <= INVALID_WAIT;
        if (KEY(15) = '1') then
            next_state <= INVALID_CODE; 
        end if;
    -- - - - - - - - - - - - - - - - - - - - - - -
    when FINISH =>
        next_state <= FINISH;
        if (KEY(15) = '1') then
            next_state <= ENTRY1; 
        end if;
    -- - - - - - - - - - - - - - - - - - - - - - -
    when others =>
        next_state <= ENTRY1;
    end case;

end process next_state_logic;

-- -------------------------------------------------------
output_logic : process(present_state, KEY)
begin
    FSM_CNT_CE     <= '0';
    FSM_MX_MEM     <= '0';
    FSM_MX_LCD     <= '0';
    FSM_LCD_WR     <= '0';
    FSM_LCD_CLR    <= '0';

    case (present_state) is
    -- - - - - - - - - - - - - - - - - - - - - - -
    when VALID_CODE =>
	    FSM_MX_MEM     <= '1';
        FSM_CNT_CE     <= '1';
        FSM_MX_LCD     <= '1';
        FSM_LCD_WR     <= '1';
    -- - - - - - - - - - - - - - - - - - - - - - -
    when INVALID_CODE =>
        FSM_CNT_CE     <= '1';
        FSM_MX_LCD     <= '1';
        FSM_LCD_WR     <= '1';
-- - - - - - - - - - - - - - - - - - - - - - -
    when FINISH =>
        if (KEY(15) = '1') then
            FSM_LCD_CLR    <= '1';
        end if;
-- - - - - - - - - - - - - - - - - - - - - - -      
    when others =>
	    if (KEY(14 downto 0) /= "000000000000000") then
            FSM_LCD_WR     <= '1';
        end if;
        if (KEY(15) = '1') then
            FSM_LCD_CLR    <= '1';
        end if;
    end case;
    
end process output_logic;

end architecture behavioral;