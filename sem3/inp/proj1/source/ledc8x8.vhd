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
    signal rows: std_logic_vector(0 to 7); 
    signal leds: std_logic_vector(0 to 7);
    signal cnt: std_logic_vector(0 to 7);
    signal en: std_logic;
begin

    --nastavime frekvenci SMCLK/256 pomoci citace
    reader: process(RESET, SMCLK) 
        begin
            if (RESET = '1') then                     
                cnt <= "00000000";
            elsif ((SMCLK'event) and (SMCLK = '1')) then 
                cnt <= cnt + 1;
            end if;
        end process reader;

    en <= '1' when cnt = "11111111" else '0';

    --rotacni registr, pomoci ktereho provedeme zmenu aktivace radku
    rows_act: process (RESET,SMCLK,en)
        begin
            if (RESET = '1') then
            rows <= "10000000";
        elsif (SMCLK'event) and (SMCLK = '1') then
            if (en='1') then
                rows <= (rows(0) & rows(7 downto 1));  
            end if;
        end if;
        
end process rows_act;

    --dekoder radku, pomoci ktereho vypocitame hodnoty signalu leds
    decoder: process (rows) is
        begin
            case (rows) is
            when "00000001" =>  leds  <= "11100000";
            when "00000010" =>  leds  <= "11111011";
            when "00000100" =>  leds  <= "01111011";
            when "00001000" =>  leds  <= "01110011";
            when "00010000" =>  leds  <= "01100011";
            when "00100000" =>  leds  <= "01010111";
            when "01000000" =>  leds  <= "00110111";
            when "10000000" =>  leds  <= "01110111";
            when others =>      leds  <= "11111111";
            end case;
    end process decoder;

    --zasvitime radky
    ROW <= rows;
    LED <= leds;

    -- Sem doplnte popis obvodu. Doporuceni: pouzivejte zakladni obvodove prvky
    -- (multiplexory, registry, dekodery,...), jejich funkce popisujte pomoci
    -- procesu VHDL a propojeni techto prvku, tj. komunikaci mezi procesy,
    -- realizujte pomoci vnitrnich signalu deklarovanych vyse.

    -- DODRZUJTE ZASADY PSANI SYNTETIZOVATELNEHO VHDL KODU OBVODOVYCH PRVKU,
    -- JEZ JSOU PROBIRANY ZEJMenA NA UVODNICH CVICenI INP A SHRNUTY NA WEBU:
    -- http://merlin.fit.vutbr.cz/FITkit/docs/navody/synth_templates.html.

    -- Nezapomente take doplnit mapovani signalu rozhrani na piny FPGA
    -- v souboru ledc8x8.ucf.

end main;
