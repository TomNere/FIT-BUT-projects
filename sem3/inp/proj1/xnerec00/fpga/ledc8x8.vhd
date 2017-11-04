library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.std_logic_arith.all;
use IEEE.std_logic_unsigned.all;

entity ledc8x8 is
port ( 
    -- definice signalu rozhrani
    ROW, LED:       out std_logic_vector (0 to 7);
    SMCLK, RESET:   in std_logic
);
end ledc8x8;

architecture main of ledc8x8 is
    -- definice vnitrnich signalu
    signal rows: std_logic_vector(7 downto 0); 
    signal leds: std_logic_vector(0 to 7);
    signal cnt: std_logic_vector(0 to 7);
    signal en: std_logic;
begin

    -- Sem doplnte popis obvodu. Doporuceni: pouzivejte zakladni obvodove prvky
    -- (multiplexory, registry, dekodery,...), jejich funkce popisujte pomoci
    -- procesu VHDL a propojeni techto prvku, tj. komunikaci mezi procesy,
    -- realizujte pomoci vnitrnich signalu deklarovanych vyse.

    -- DODRZUJTE ZASADY PSANI SYNTETIZOVATELNEHO VHDL KODU OBVODOVYCH PRVKU,
    -- JEZ JSOU PROBIRANY ZEJMenA NA UVODNICH CVICenI INP A SHRNUTY NA WEBU:
    -- http://merlin.fit.vutbr.cz/FITkit/docs/navody/synth_templates.html.

    -- Nezapomente take doplnit mapovani signalu rozhrani na piny FPGA
    -- v souboru ledc8x8.ucf.

    ---------------------------------------------------------------------------

    --prirazeni vnitrnich signalu k signalum rozhrani
    ROW <= rows;
    LED <= leds;

    --nastaveni frekvence SMCLK/256 pomoci citace s asynchronnim nulovanim (FITkit navody 2.4, 2.2)
    set_freq: process(RESET, SMCLK) 
        begin
            if (RESET = '1') then                     
                cnt <= (others => '0');
            elsif ((SMCLK'event) and (SMCLK = '1')) then 
                cnt <= cnt + 1;
            end if;
    end process set_freq;

    --radek bude aktivni 256 period, pak se radek zmeni
    en <= '1' when cnt = "11111111" else '0';

    --posuvny registr, pomoci ktereho provedeme zmenu aktivace radku (FITkit navody 2.3)
    act_rows: process (RESET,SMCLK)
        begin
            if (RESET = '1') then
                rows <= "00000001";
            elsif (SMCLK'event) and (SMCLK = '1') then
                if (en='1') then
                    rows <= rows(0) & rows(7 downto 1);  
                end if;
            end if;    
	end process act_rows;

    --dekoder radku, pomoci ktereho vypocitame hodnoty signalu leds (FITkit navody 1.3)
    --1 v rows znamena, ze je radek aktivni, 0 v leds znamena, ze ledka sviti
    decoder: process (rows) is
        begin
        	leds <= (others => '0');
            case (rows) is
            when "00000001" =>  leds  <= "11110110";    
            when "00000010" =>  leds  <= "11110100";    
            when "00000100" =>  leds  <= "11110010";    
            when "00001000" =>  leds  <= "11010110";    
            when "00010000" =>  leds  <= "11011111";    
            when "00100000" =>  leds  <= "11011111";    
            when "01000000" =>  leds  <= "11011111";    
            when "10000000" =>  leds  <= "00000111";    
            when others =>      leds  <= (others => '1');
            end case;
    end process decoder;

end main;
