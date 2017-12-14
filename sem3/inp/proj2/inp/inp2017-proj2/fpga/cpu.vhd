-- cpu.vhd: Simple 8-bit CPU (BrainLove interpreter)
-- Copyright (C) 2017 Brno University of Technology,
--                    Faculty of Information Technology
-- Author(s): DOPLNIT
--

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;

-- ----------------------------------------------------------------------------
--                        Entity declaration
-- ----------------------------------------------------------------------------
entity cpu is
 port (
   CLK   : in std_logic;  -- hodinovy signal
   RESET : in std_logic;  -- asynchronni reset procesoru
   EN    : in std_logic;  -- povoleni cinnosti procesoru
 
   -- synchronni pamet ROM
   CODE_ADDR : out std_logic_vector(11 downto 0); -- adresa do pameti
   CODE_DATA : in std_logic_vector(7 downto 0);   -- CODE_DATA <- rom[CODE_ADDR] pokud CODE_EN='1'
   CODE_EN   : out std_logic;                     -- povoleni cinnosti
   
   -- synchronni pamet RAM
   DATA_ADDR  : out std_logic_vector(9 downto 0); -- adresa do pameti
   DATA_WDATA : out std_logic_vector(7 downto 0); -- mem[DATA_ADDR] <- DATA_WDATA pokud DATA_EN='1'
   DATA_RDATA : in std_logic_vector(7 downto 0);  -- DATA_RDATA <- ram[DATA_ADDR] pokud DATA_EN='1'
   DATA_RDWR  : out std_logic;                    -- cteni z pameti (DATA_RDWR='0') / zapis do pameti (DATA_RDWR='1')
   DATA_EN    : out std_logic;                    -- povoleni cinnosti
   
   -- vstupni port
   IN_DATA   : in std_logic_vector(7 downto 0);   -- IN_DATA obsahuje stisknuty znak klavesnice pokud IN_VLD='1' a IN_REQ='1'
   IN_VLD    : in std_logic;                      -- data platna pokud IN_VLD='1'
   IN_REQ    : out std_logic;                     -- pozadavek na vstup dat z klavesnice
   
   -- vystupni port
   OUT_DATA : out  std_logic_vector(7 downto 0);  -- zapisovana data
   OUT_BUSY : in std_logic;                       -- pokud OUT_BUSY='1', LCD je zaneprazdnen, nelze zapisovat,  OUT_WE musi byt '0'
   OUT_WE   : out std_logic                       -- LCD <- OUT_DATA pokud OUT_WE='1' a OUT_BUSY='0'
 );
end cpu;


-- ----------------------------------------------------------------------------
--                      Architecture declaration
-- ----------------------------------------------------------------------------
architecture behavioral of cpu is

 -- zde dopiste potrebne deklarace signalu
signal pc_reg : std_logic_vector(15 downto 0);
signal pc_dec : std_logic;
signal pc_inc : std_logic;

signal ptr_reg : std_logic_vector(15 downto 0);
signal ptr_dec : std_logic;
signal ptr_inc : std_logic;

signal ptr_reg : std_logic_vector(15 downto 0);
signal ptr_dec : std_logic;
signal ptr_inc : std_logic;

---------------------------------------
-- Instruction
type instruction_type is (
   pointer_inc, pointer_dec,
   value_inc, value_dec,
   while_begin, while_end,
   value_print, value_read,
   break, terminate,
   nothing
);
signal instruction : instruction_type;
------------------------------------------
begin

------------------------------------
-- PC register - program counter
pc_reg: process (RESET, CLK)
begin
   if (RESET = '1') then
      pc_reg <= (others => '0');
   elsif(CLK'event) and (CLK = '1') then
      if (pc_dec = '1') then
         pc_reg <= pc_reg - 1;
      elsif(pc_inc = '1') then
         pc_reg <= pc_reg + 1;
      end if;
   end if;
end process;

CODE_ADDR <= pc_reg;

-------------------------------------
-- PTR register - pointer to data
ptr_reg: process (RESET, CLK)
begin
   if (RESET = '1') then
      ptr_reg <= (others => '0');
   elsif(CLK'event) and (CLK = '1') then
      if (ptr_dec = '1') then
         ptr_reg <= ptr_reg - 1;
      elsif(ptr_inc = '1') then
         ptr_reg <= ptr_reg + 1;
      end if;
   end if;
end process;

DATA_ADDR <= ptr_reg;

-------------------------------------
-- CNT register - while counter
cnt_reg: process (RESET, CLK)
begin
   if (RESET = '1') then
      cnt_reg <= (others => '0');
   elsif(CLK'event) and (CLK = '1') then
      if (cnt_dec = '1') then
         cnt_reg <= cnt_reg - 1;
      elsif(cnt_inc = '1') then
         cnt_reg <= cnt_reg + 1;
      end if;
   end if;
end process;

DATA_ADDR <= ptr_data;

-- Multiplexer
with sel select
   DATA_WDATA <= IN_DATA when "00",
               data_rdata_plus when "01",
               data_rdata_minus when "10",
               (others => '0') when others;

cnt_data_aux <= '1' when (cnt_data = "00000000") else '0';
data_rdata_aux <= '1' when (DATA_RDATA = "00000000") else '0';

--------------------------------------
-- Instruction decoder
instr_decoder: process (DATA_RDATA)
begin
   case (DATA_RDATA) is
      when X"3E"  => instruction <= pointer_inc; 
      when X"3C"  => instruction <= pointer_dec; 
      when X"2B"  => instruction <= value_inc; 
      when X"2D"  => instruction <= value_dec; 
      when X"5B"  => instruction <= while_begin;
      when X"5D"  => instruction <= while_end;
      when X"2E"  => instruction <= value_print; 
      when X"2C"  => instruction <= value_read;
      when X"7E"  => instruction <= break; 
      when X"00"  => instruction <= terminate;
      when others => instruction <= nothing; 
   end case;
end process;

end behavioral;
 
