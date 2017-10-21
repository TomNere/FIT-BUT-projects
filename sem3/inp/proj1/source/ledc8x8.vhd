library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.std_logic_arith.all;
use IEEE.std_logic_unsigned.all;

entity ledc8x8 is
port ( -- Sem doplnte popis rozhrani obvodu.
    ROW, LED:       out std_logic_vector (0 to 7);
    SMCLK, RESET:   in std_logic
);
end ledc8x8;

architecture main of ledc8x8 is

    -- Sem doplnte definice vnitrnich signalu.
    signal rows: std_logic_vector(7 downto 0); --vnutorne signaly
    signal cols: std_logic_vector(7 downto 0);
    signal ce_cnt: std_logic_vector(7 downto 0);
    signal ce: std_logic;
begin
    --snizime frekvenci
    generator_ce: process(SMCLK, RESET) -- odvodenie od SMCLK
        begin
            if (RESET = '1') then                     --asynchrony reset
                ce_cnt <= "00000000";
            elsif ((SMCLK'event) and (SMCLK = '1')) then --even nastupna hrana
                ce_cnt <= ce_cnt + 1;
            end if;
        end process generator_ce;

    ce <= '1' when ce_cnt = "11111111" else '0';

    --rotacni registr
    rows_cnt: process (RESET,SMCLK,ce)
        begin
            if (RESET = '1') then
            rows <= "11111110";
        elsif ((SMCLK'event) and (SMCLK = '1') and (ce = '1')) then
            rows <= row_cnt(0) & row_cnt(7 downto 1);  --konkatenacia na pusunutie jednotky
        end if;
        
end process rows_cnt;

--this is decoder for my name
    dec: process (row_cnt) is
    begin
        case (row_cnt) is
        when "01111111" =>  name  <= "10001000";
        when "10111111" =>  name  <= "11001000";
        when "11011111" =>  name  <= "10101000";
        when "11101111" =>  name  <= "10011100";
        when "11110111" =>  name  <= "10001100";
        when "11111011" =>  name  <= "10000100";
        when "11111101" =>  name  <= "00000100";
        when "11111110" =>  name  <= "00011111";
        when others =>      name  <= "00000000";
        end case;
    end process dec;
    -- Sem doplnte popis obvodu. Doporuceni: pouzivejte zakladni obvodove prvky
    -- (multiplexory, registry, dekodery,...), jejich funkce popisujte pomoci
    -- procesu VHDL a propojeni techto prvku, tj. komunikaci mezi procesy,
    -- realizujte pomoci vnitrnich signalu deklarovanych vyse.

    -- DODRZUJTE ZASADY PSANI SYNTETIZOVATELNEHO VHDL KODU OBVODOVYCH PRVKU,
    -- JEZ JSOU PROBIRANY ZEJMENA NA UVODNICH CVICENI INP A SHRNUTY NA WEBU:
    -- http://merlin.fit.vutbr.cz/FITkit/docs/navody/synth_templates.html.

    -- Nezapomente take doplnit mapovani signalu rozhrani na piny FPGA
    -- v souboru ledc8x8.ucf.

end main;
