//    Copyright Julian van Doorn and Wiebe van Breukelen.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <system_sam3xa.h>
#include <sam3x8e.h>
#include <tuple>
#include <utility>
#include <algorithm>

namespace hwstl {
    using pin_index = uint32_t;

    template <pin_index... vt_pins>
    using pin_sequence = std::integer_sequence<pin_index, vt_pins...>;

    namespace arduino_due {
#ifdef DEBUG
        static inline void debug_assert(bool assertion_success, const char* error) {
            if (!assertion_success) {
            // block execution, throw error with debug info
            }
        }
#else
        static inline void debug_assert(bool assertion_success, const char* error) {
            // Do nothing during execution, code should be designed so that
            // this function is never called. It's advised to compile with
            // debug to test these assertions.
        }
#endif

        namespace internal {

        };

        void init();

        namespace pin {
            class pin_info {
            public:
                uint8_t m_port;
                uint8_t m_pin;

                constexpr pin_info(uint8_t port, uint8_t pin) : m_port(port), m_pin(pin) { }
            };

            static constexpr pin_info pin_info_array[21] = {
                { 0,  8 },  // d0
                { 0,  9 },  // d1
                { 1, 25 },  // d2
                { 2, 28 },  // d3
                { 2, 26 },  // d4
                { 2, 25 },  // d5
                { 2, 24 },  // d6
                { 2, 23 },  // d7
                { 2, 22 },  // d8
                { 2, 21 },  // d9
                { 2, 29 },  // d10

                { 3,  7 },  // d11
                { 3,  8 },  // d12
                { 1, 27 },  // d13
                { 3,  4 },  // d14
                { 3,  5 },  // d15
                { 0, 13 },  // d16
                { 0, 12 },  // d17
                { 0, 11 },  // d18
                { 0, 10 },  // d19
                { 1, 12 },  // d20
            };

            template <pin_index t_pin>
            __attribute__((always_inline))
            static inline constexpr Pio* GetPortByPin() {
                uint8_t port = pin_info_array[t_pin].m_port;

                if (port == 0) {
                    return PIOA;
                } else if (port == 1) {
                    return PIOB;
                } else if (port == 2) {
                    return PIOC;
                } else if (port == 3) {
                    return PIOD;
                } else {
                    return nullptr;
                }
            }

            template <pin_index t_pin>
            __attribute__((always_inline))
            static inline constexpr uint32_t GetPinInPort() {
                return pin_info_array[t_pin].m_pin;
            }

            template <pin_index t_pin>
            __attribute__((always_inline))
            static inline constexpr uint32_t GetPinMask() {
                return 1 << GetPinInPort<t_pin>();
            }

            template <pin_index t_pin>
            __attribute__((always_inline))
            static inline int PinEnable() {
                Pio* port = GetPortByPin<t_pin>();
                uint32_t mask = 1 << GetPinInPort<t_pin>();

                port->PIO_PER = mask;
                port->PIO_OER = mask;

                return 1;
            }

            template <pin_index t_pin>
            __attribute__((always_inline))
            static inline constexpr void ProcessPinEntry(uint32_t masks[4]) {
                uint8_t port = pin_info_array[t_pin].m_port;

                if (port == 0) {
                    masks[0] |= GetPinMask<t_pin>(); 
                } else if (port == 1) {
                    masks[1] |= GetPinMask<t_pin>(); 
                } else if (port == 2) {
                    masks[2] |= GetPinMask<t_pin>();
                } else if (port == 3) {
                    masks[3] |= GetPinMask<t_pin>();
                }
            }

            template <pin_index... vt_pins>
            __attribute__((always_inline))
            static inline void PinSequenceEnable(pin_sequence<vt_pins...> pins) {
                uint32_t masks[4]   = {};
                uint32_t pmc0_enable = 0;

                (ProcessPinEntry<vt_pins>(masks), ...);

                if (masks[0]) {
                    pmc0_enable   |= (1 << 11);

                    PIOA->PIO_PER = masks[0];
                    PIOA->PIO_OER = masks[0];
                }

                if (masks[1]) {
                    pmc0_enable   |= (1 << 12);

                    PIOB->PIO_PER = masks[1];
                    PIOB->PIO_OER = masks[1];
                }
                
                if (masks[2]) {
                    pmc0_enable   |= (1 << 13);

                    PIOC->PIO_PER = masks[2];
                    PIOC->PIO_OER = masks[2];
                }

                if (masks[3]) {
                    pmc0_enable   |= (1 << 14);

                    PIOD->PIO_PER = masks[3];
                    PIOD->PIO_OER = masks[3];
                }

                PMC->PMC_PCER0 = pmc0_enable;
            }

            template <pin_index... vt_pins>
            __attribute__((always_inline))
            static inline void configure_in(pin_sequence<vt_pins...> pins) {
                PinSequenceEnable(pins);
            }

            template <pin_index... vt_pins>
            __attribute__((always_inline))
            static inline void configure_out(pin_sequence<vt_pins...> pins) {
                PinSequenceEnable(pins);
            }

            template <pin_index... vt_pins>
            __attribute__((always_inline))
            static inline void configure_inout(pin_sequence<vt_pins...> pins) {
                PinSequenceEnable(pins);
            }

            template <pin_index t_pin>
            class pin_impl {
            public:
                static constexpr pin_index pin = t_pin;

                constexpr pin_impl() { }

                static inline void set(bool v) {
                    Pio* port = GetPortByPin<t_pin>();
                    uint32_t mask = GetPinMask<t_pin>();

                    (v ? port->PIO_SODR : port->PIO_CODR) = mask;
                }

                static inline bool get() {
                    Pio* port = GetPortByPin<t_pin>();
                    uint32_t mask = GetPinMask<t_pin>();

                    return (port->PIO_PDSR & mask) != 0;
                }

                static inline void enable_pullup() {
                    Pio* port = GetPortByPin<t_pin>();
                    uint32_t mask = GetPinMask<t_pin>();
                    
                    port->PIO_PUER = mask;	
                }

                static inline void disable_pullup() {
                    Pio* port = GetPortByPin<t_pin>();
                    uint32_t mask = GetPinMask<t_pin>();
                    
                    port->PIO_PUDR = mask;
                }
            };

#ifdef HWSTL_ONCE
            auto d0 = pin_impl<0>();
            auto d1 = pin_impl<1>();
            auto d2 = pin_impl<2>();
            auto d7 = pin_impl<7>();
            auto d13 = pin_impl<13>();
#endif
        };

        enum class uart_io {
            uart,
            usart0,
            usart1,
            usart2
        };

        static constexpr uint32_t MainClockFrequency = 84000000;

        namespace uart_util {
            enum class USARTMode {
                Normal = 0,
                RS485,
                HardwareHandshaking,
                ISO7816_T_0 = 4,
                ISO7816_T_1 = 6,
                IrDA = 8,
                LinMaster = 0xA,
                LinSlave,
                SPIMaster = 0xE,
                SPISlave
            };

            enum class ClockSelection {
                MasterClock = 0,
                Divided,
                SerialClock = 3
            };

            class CharacterLength {
            public:
                unsigned charLength : 3;

                constexpr CharacterLength() : charLength(5) { }
                constexpr CharacterLength(uint8_t charLength) : charLength(charLength) { }
            };

            static constexpr int32_t GetCharacterLength(uint32_t bits) {
                if (bits == 5) {
                    return 0;
                } else if (bits == 6) {
                    return 1;
                } else if (bits == 7) {
                    return 2;
                } else if (bits == 8) {
                    return 3;
                } else if (bits == 9) {
                    return 4; // THIS CASE SHOULD BE HANDLED DIFFERENTLY, SEE DATASHEET FOR USART "MODE9"
                }

                return -1;
            }

            enum class SynchronousMode {
                Async = 0,
                Sync = 1
            };

            enum class SPIClockPhase {
                // Needs more elaboration, and perhals a more descriptive enum
                LeadingEdgeChanged_FollowingEdgeCaptured = 0,
                LeadingEdgeCaptured_FollowingEdgeChanged = 1
            };

            enum class Parity {
                Even = 0,
                Odd,
                Space,
                Mark,
                No
            };

            enum class StopBits {
                OneBit = 0,
                OneAndHalfBits,
                TwoBits
            };

            enum class Channel {
                Normal = 0,
                Automatic,
                LocalLoopback,
                RemoteLoopback
            };

            enum class BitOrder {
                LeastSignificantFirst = 0,
                MostSignificantFirst = 1
            };

            enum class SPIClockPolarity {
                ActiveHigh = 0,
                ActiveLow = 1
            };

            enum class ClockOutput {
                NotDriven = 0,
                Driven = 1
            };

            enum class OversamplingMode {
                x16 = 0,
                x8 = 1
            };

            enum class InhibitNonAcknowledge {
                Generated = 0,
                NotGenerated = 1
            };

            enum class SuccessiveNACK {
                // TODO, see datasheet USART DSNACK
            };

            enum class InvertedData {
                ActiveHigh = 0,
                ActiveLow = 1
            };

            enum class VariableSynchronization {
                UserDefined = 0,
                OnReceived = 1
            };

            class MaxIterations {
            public:
                unsigned maxIterations : 3;

                constexpr MaxIterations(uint8_t maxIterations) : maxIterations(maxIterations) { }
            };

            enum class InfraredRxFilter {
                None = 0,
                ThreeSampleFilter = 1
            };

            enum class ManchesterCodecEnabled {
                Disabled = 0,
                Enabled = 1
            };

            enum class ManchesterSynchronizationMode {
                LowToHighTransition = 0,
                HighToLowTransition = 1
            };

            enum class StartFrameDelimiter {
                CommandOrDataSync = 0,
                OneBit = 1
            };

            template <class t_type, t_type t_uart>
            class Mode;

            template <Uart* t_uart>
            class Mode<Uart*, t_uart> {
            public:
                Parity parity;
                Channel channel;

                constexpr Mode() { }

                constexpr Mode(
                    Parity parity = Parity::Even,
                    Channel channel = Channel::Normal
                ) : 
                    parity(parity),
                    channel(channel)
                { }
            };

            template <Usart* t_usart>
            class Mode<Usart*, t_usart> {
            public:
                USARTMode mode;
                ClockSelection clockSelection;
                CharacterLength characterLength;
                SynchronousMode synchronousMode;
                SPIClockPhase spiClockPhase;
                Parity parity;
                StopBits stopBits;
                Channel channel;
                BitOrder bitOrder;
                SPIClockPolarity spiClockPolarity;
                ClockOutput clockOutput;
                OversamplingMode oversamplingMode;
                InhibitNonAcknowledge inhibitNonAcknowledge;
                SuccessiveNACK successiveNACK;
                InvertedData invertedData;
                VariableSynchronization variableSynchronization;
                MaxIterations maxIterations;
                InfraredRxFilter infraredRxFilter;
                ManchesterCodecEnabled manchesterCodecEnabled;
                ManchesterSynchronizationMode manchesterSynchronizationMode;
                StartFrameDelimiter startFrameDelimiter;

                constexpr Mode(
                    USARTMode mode = USARTMode::Normal,
                    ClockSelection clockSelection = ClockSelection::MasterClock,
                    CharacterLength characterLength = CharacterLength(),
                    SynchronousMode synchronousMode = SynchronousMode::Async,
                    SPIClockPhase spiClockPhase = SPIClockPhase::LeadingEdgeChanged_FollowingEdgeCaptured,
                    Parity parity = Parity::Even,
                    StopBits stopBits = StopBits::OneBit,
                    Channel channel = Channel::Normal,
                    BitOrder bitOrder = BitOrder::LeastSignificantFirst,
                    SPIClockPolarity spiClockPolarity = SPIClockPolarity::ActiveHigh,
                    ClockOutput clockOutput = ClockOutput::NotDriven,
                    OversamplingMode oversamplingMode = OversamplingMode::x16,
                    InhibitNonAcknowledge inhibitNonAcknowledge = InhibitNonAcknowledge::Generated,
                    SuccessiveNACK successiveNACK = 0,
                    InvertedData invertedData = InvertedData::ActiveHigh,
                    VariableSynchronization variableSynchronization = VariableSynchronization::UserDefined,
                    MaxIterations maxIterations = MaxIterations(),
                    InfraredRxFilter infraredRxFilter = InfraredRxFilter::None,
                    ManchesterCodecEnabled manchesterCodecEnabled = ManchesterCodecEnabled::Disabled,
                    ManchesterSynchronizationMode manchesterSynchronizationMode = ManchesterSynchronizationMode::LowToHighTransition,
                    StartFrameDelimiter startFrameDelimiter = StartFrameDelimiter::CommandOrDataSync
                ) :
                    mode(mode),
                    clockSelection(clockSelection),
                    characterLength(characterLength),
                    synchronousMode(synchronousMode),
                    spiClockPhase(spiClockPhase),
                    parity(parity),
                    stopBits(stopBits),
                    channel(channel),
                    bitOrder(bitOrder),
                    spiClockPolarity(spiClockPolarity),
                    clockOutput(clockOutput),
                    oversamplingMode(oversamplingMode),
                    inhibitNonAcknowledge(inhibitNonAcknowledge),
                    successiveNACK(successiveNACK),
                    invertedData(invertedData),
                    variableSynchronization(variableSynchronization),
                    maxIterations(maxIterations),
                    manchesterCodecEnabled(manchesterCodecEnabled),
                    manchesterSynchronizationMode(manchesterSynchronizationMode),
                    startFrameDelimiter(startFrameDelimiter)
                { }
            };

            static constexpr int32_t FromFrequencyToPrescalerSelector(uint32_t main_clock_frequency) {
                // 0 CLK Selected clock
                // 1 CLK_2 Selected clock divided by 2
                // 2 CLK_4 Selected clock divided by 4
                // 3 CLK_8 Selected clock divided by 8
                // 4 CLK_16 Selected clock divided by 16
                // 5 CLK_32 Selected clock divided by 32
                // 6 CLK_64 Selected clock divided by 64
                // 7 CLK_3 Selected clock divided by 3

                if (main_clock_frequency == MainClockFrequency) {
                    return 0;
                } else if (main_clock_frequency == MainClockFrequency / 2) {
                    return 1;
                } else if (main_clock_frequency == MainClockFrequency / 4) {
                    return 2;
                } else if (main_clock_frequency == MainClockFrequency / 8) {
                    return 3;
                } else if (main_clock_frequency == MainClockFrequency / 16) {
                    return 4;
                } else if (main_clock_frequency == MainClockFrequency / 32) {
                    return 5;
                } else if (main_clock_frequency == MainClockFrequency / 64) {
                    return 6;
                } else if (main_clock_frequency == MainClockFrequency / 3) {
                    return 7;
                } else {
                    return -1; // Invalid clock frequency
                }
            }

            static constexpr int32_t FromPrescalerSelectorToFrequency(uint32_t prescaler_selection) {
                if (prescaler_selection == 0) {
                    return MainClockFrequency;
                } else if (prescaler_selection == 1) {
                    return MainClockFrequency / 2;
                } else if (prescaler_selection == 2) {
                    return MainClockFrequency / 4;
                } else if (prescaler_selection == 3) {
                    return MainClockFrequency / 8;
                } else if (prescaler_selection == 4) {
                    return MainClockFrequency / 16;
                } else if (prescaler_selection == 5) {
                    return MainClockFrequency / 32;
                } else if (prescaler_selection == 6) {
                    return MainClockFrequency / 64;
                } else if (prescaler_selection == 7) {
                    return MainClockFrequency / 3;
                } else {
                    return -1; // Invalid prescaler
                }
            }

            /**
             * @brief Checks if the baudrate can be exactly generated
             * 
             * @details
             * According to the SAM3X datasheet, the baudrate shoud be
             * divisible by 16. This can by tested by checking if the last four
             * are 0. 
             * 
             * @param baudrate 
             * @return true Baudrate can be generated by hardware
             * @return false Invalid baudrate
             */
            static constexpr bool IsValidBaudrate(uint32_t baudrate) {
                return (baudrate & 0b1111) == 0;
            }

            /**
             * @brief Calculates the divider for UART_BRGR
             * 
             * @details
             * Uses the formula CD = (MCK / BAUD) / 16 to select the right
             * clock divider. That formula is derived from the formula
             * provided by the SAM3X datasheet: BAUD = MCK / (CD * 16)
             * 
             * @param master_clock_frequency 
             * @param baudrate 
             */
            static inline uint32_t CalculateDivider(uint32_t master_clock_frequency, uint32_t baudrate) {
                return (master_clock_frequency / baudrate) / 16;
            }

            /**
             * @brief Enables baud generation with the given clock
             * frequency and baudrate
             * 
             * @tparam t_master_clock_frequency 
             * @tparam t_baudrate 
             */
            template <uint32_t t_master_clock_frequency, uint32_t t_baudrate, class t_type>
            static inline void EnableBaud(t_type uart) {
                static_assert(FromFrequencyToPrescalerSelector(t_master_clock_frequency) != -1, "Invalid master clock frequency");
                static_assert(IsValidBaudrate(t_baudrate), "Invalid baudrate");
                uart->UART_BRGR = CalculateDivider(t_master_clock_frequency, t_baudrate);
            }

            /**
             * @brief Enables baud generation for the given baud
             * 
             * @details
             * This function is designed to change baudrate during runtime.
             * t_master_clock_frequency is provided during compilation since
             * the CPU clock speed generally doesn't change during
             * operation. Else EnableBaud(master_clock_frequency, baudrate)
             * should be used.
             * 
             * @tparam t_master_clock_frequency 
             * @param baudrate 
             */
            template <uint32_t t_master_clock_frequency, class t_type, t_type t_uart>
            static inline void EnableBaud(t_type uart, uint32_t baudrate) {
                static_assert(FromFrequencyToPrescalerSelector(t_master_clock_frequency) != -1, "Invalid master clock frequency");
                debug_assert(IsValidBaudrate(baudrate), "Invalid baudrate");
                t_uart->UART_BRGR = CalculateDivider(t_master_clock_frequency, baudrate);
            }

            /**
             * @brief Enables baud generation for the given baud
             * 
             * @details
             * This function is designed to change the baudrate during runtime
             * without a predetermined master clock frequency. That means when
             * using this function the MCK must be looked up or calculated.
             * 
             * @param master_clock_frequency 
             * @param baudrate 
             */
            template <class t_type>
            static inline void EnableBaud(t_type uart, uint32_t master_clock_frequency, uint32_t baudrate) {
                debug_assert(FromFrequencyToPrescalerSelector(master_clock_frequency) != -1, "Invalid master clock frequency");
                debug_assert(IsValidBaudrate(baudrate), "Invalid baudrate");
                uart->UART_BRGR = CalculateDivider(master_clock_frequency, baudrate);
            }

            /**
             * @brief Disable baud generation for UART
             */
            template <class t_type>
            static inline void DisableBaud(t_type uart) {
                // Zero for disabling
                uart->UART_BRGR = 0;
            }

            template <class t_type>
            static inline void ResetTRX(t_type uart);

            /**
             * @brief Resets the UART Tx and Rx
             */
            template <>
            void ResetTRX<Uart*>(Uart* uart) {
                uart->UART_CR = UART_CR_RSTRX | UART_CR_RSTTX | UART_CR_RXDIS | UART_CR_TXDIS;
            }

            /**
             * @brief Resets the USART Tx and Rx
             */
            template <>
            void ResetTRX<Usart*>(Usart* usart) {
                usart->US_CR = UART_CR_RSTRX | UART_CR_RSTTX | UART_CR_RXDIS | UART_CR_TXDIS;
            }

            template <class t_type>
            static inline void EnableTRX(t_type uart);

            /**
             * @brief Enables the UART Tx and Rx
             */
            template <>
            void EnableTRX<Uart*>(Uart* uart) {
                uart->UART_CR = UART_CR_RXEN | UART_CR_TXEN; 
            }

            /**
             * @brief Enables the USART Tx and Rx
             */
            template <>
            void EnableTRX<Usart*>(Usart* usart) {
                usart->US_CR = UART_CR_RXEN | UART_CR_TXEN;
            }
        };

        template <uart_io t_uart>
        class uart_impl;

        /**
         * @brief Primary UART controller
         * 
         * @details
         * Primary UART controller routines. All debug logs should by default
         * be sent using the routines in this class.
         */
        template <>
        class uart_impl<uart_io::uart> {

        public:
            static inline void Enable() {
                // enable the clock to port A
                PMC->PMC_PCER0 = 1 << ID_PIOA;

                // disable PIO Control on PA9 and set up for Peripheral A
                PIOA->PIO_PDR = PIO_PA8; 
                PIOA->PIO_ABSR &= ~PIO_PA8;
                PIOA->PIO_PDR = PIO_PA9; 
                PIOA->PIO_ABSR &= ~PIO_PA9;

                // enable the clock to the UART
                PMC->PMC_PCER0 = 0x01 << ID_UART;

                // Reset and disable receiver and transmitter.
                uart_util::ResetTRX(UART);
                uart_util::EnableBaud<MainClockFrequency, 115200>(UART);

                // No parity, normal channel mode.
                UART->UART_MR = UART_MR_PAR_NO;

                // Disable all interrupts.	  
                UART->UART_IDR = 0xFFFFFFFF;   

                uart_util::EnableTRX(UART);
            }

            static inline void Disable() {
                // Disable PIO control on PA8 and PA9
                // Leaves peripheral mode undefined
                PIOA->PIO_PDR = PIO_PA8;
                PIOA->PIO_PDR = PIO_PA9;

                // Disable clock to the UART
                PMC->PMC_PCER0 = 0x01 << ID_UART;
            }

            /**
             * @brief Places char c on the UART Tx line
             * 
             * @param c 
             */
            static inline void putc(unsigned char c) {
                while((UART->UART_SR & 2) == 0) { }
                UART->UART_THR = c;
            }

            /**
             * @brief Reads char from the UART Rx line
             * 
             * @return unsigned char 
             */
            static inline unsigned char getc() {
                while((UART->UART_SR & 1) == 1) { }
                return UART->UART_RHR; 
            }

            /**
             * @brief Reads char from the UART Rx line with a timeout
             * 
             * @details
             * Returns -1 when a timeout occurred. Safe to cast to unsigned
             * char if getc did not timeout.
             * 
             * @return int32_t
             */
            template <uint32_t t_timeout>
            static inline int32_t getc() {
                while(UART->UART_SR & 1 == 1) { } // TODO: Check for timeout and return -1
                return UART->UART_RHR;
            }

            inline static void Configure()  { 
                Enable();
            }
        };

        uint_fast64_t now_ticks();

        uint64_t now_us();

        void wait_us_busy(int_fast32_t n);

        void wait_us(int_fast32_t n);

        void wait_ms(int_fast32_t n);
    } // namespace arduino_due
} // namespace hwstl