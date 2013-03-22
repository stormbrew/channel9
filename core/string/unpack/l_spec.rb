require File.expand_path('../../../../spec_helper', __FILE__)
require File.expand_path('../../fixtures/classes', __FILE__)
require File.expand_path('../shared/integer', __FILE__)

little_endian do
  describe "String#unpack with format 'L'" do
    it_behaves_like :string_unpack_basic, 'L'
    it_behaves_like :string_unpack_32bit_le, 'L'
    it_behaves_like :string_unpack_32bit_le_unsigned, 'L'
  end

  describe "String#unpack with format 'l'" do
    it_behaves_like :string_unpack_basic, 'l'
    it_behaves_like :string_unpack_32bit_le, 'l'
    it_behaves_like :string_unpack_32bit_le_signed, 'l'
  end

  platform_is :wordsize => 32 do
    describe "String#unpack with format 'L' with modifier '_'" do
      it_behaves_like :string_unpack_32bit_le, 'L_'
      it_behaves_like :string_unpack_32bit_le_unsigned, 'L_'
    end

    describe "String#unpack with format 'L' with modifier '!'" do
      it_behaves_like :string_unpack_32bit_le, 'L!'
      it_behaves_like :string_unpack_32bit_le_unsigned, 'L!'
    end

    describe "String#unpack with format 'l' with modifier '_'" do
      it_behaves_like :string_unpack_32bit_le, 'l_'
      it_behaves_like :string_unpack_32bit_le_signed, 'l'
    end

    describe "String#unpack with format 'l' with modifier '!'" do
      it_behaves_like :string_unpack_32bit_le, 'l!'
      it_behaves_like :string_unpack_32bit_le_signed, 'l'
    end
  end

  platform_is :wordsize => 64 do
    platform_is_not :os => :windows do
      describe "String#unpack with format 'L' with modifier '_'" do
        it_behaves_like :string_unpack_64bit_le, 'L_'
        it_behaves_like :string_unpack_64bit_le_unsigned, 'L_'
      end

      describe "String#unpack with format 'L' with modifier '!'" do
        it_behaves_like :string_unpack_64bit_le, 'L!'
        it_behaves_like :string_unpack_64bit_le_unsigned, 'L!'
      end

      describe "String#unpack with format 'l' with modifier '_'" do
        it_behaves_like :string_unpack_64bit_le, 'l_'
        it_behaves_like :string_unpack_64bit_le_signed, 'l_'
      end

      describe "String#unpack with format 'l' with modifier '!'" do
        it_behaves_like :string_unpack_64bit_le, 'l!'
        it_behaves_like :string_unpack_64bit_le_signed, 'l!'
      end
    end

    platform_is :os => :windows do
      not_compliant_on :jruby do
        describe "String#unpack with format 'L' with modifier '_'" do
          it_behaves_like :string_unpack_32bit_le, 'L_'
          it_behaves_like :string_unpack_32bit_le_unsigned, 'L_'
        end

        describe "String#unpack with format 'L' with modifier '!'" do
          it_behaves_like :string_unpack_32bit_le, 'L!'
          it_behaves_like :string_unpack_32bit_le_unsigned, 'L!'
        end

        describe "String#unpack with format 'l' with modifier '_'" do
          it_behaves_like :string_unpack_32bit_le, 'l_'
          it_behaves_like :string_unpack_32bit_le_signed, 'l_'
        end

        describe "String#unpack with format 'l' with modifier '!'" do
          it_behaves_like :string_unpack_32bit_le, 'l!'
          it_behaves_like :string_unpack_32bit_le_signed, 'l!'
        end
      end

      deviates_on :jruby do
        describe "String#unpack with format 'L' with modifier '_'" do
          it_behaves_like :string_unpack_64bit_le, 'L_'
          it_behaves_like :string_unpack_64bit_le_unsigned, 'L_'
        end

        describe "String#unpack with format 'L' with modifier '!'" do
          it_behaves_like :string_unpack_64bit_le, 'L!'
          it_behaves_like :string_unpack_64bit_le_unsigned, 'L!'
        end

        describe "String#unpack with format 'l' with modifier '_'" do
          it_behaves_like :string_unpack_64bit_le, 'l_'
          it_behaves_like :string_unpack_64bit_le_signed, 'l_'
        end

        describe "String#unpack with format 'l' with modifier '!'" do
          it_behaves_like :string_unpack_64bit_le, 'l!'
          it_behaves_like :string_unpack_64bit_le_signed, 'l!'
        end
      end
    end
  end
end

big_endian do
  describe "String#unpack with format 'L'" do
    it_behaves_like :string_unpack_basic, 'L'
    it_behaves_like :string_unpack_32bit_be, 'L'
    it_behaves_like :string_unpack_32bit_be_unsigned, 'L'
  end

  describe "String#unpack with format 'l'" do
    it_behaves_like :string_unpack_basic, 'l'
    it_behaves_like :string_unpack_32bit_be, 'l'
    it_behaves_like :string_unpack_32bit_be_signed, 'l'
  end

  platform_is :wordsize => 32 do
    describe "String#unpack with format 'L' with modifier '_'" do
      it_behaves_like :string_unpack_32bit_be, 'L_'
      it_behaves_like :string_unpack_32bit_be_unsigned, 'L_'
    end

    describe "String#unpack with format 'L' with modifier '!'" do
      it_behaves_like :string_unpack_32bit_be, 'L!'
      it_behaves_like :string_unpack_32bit_be_unsigned, 'L!'
    end

    describe "String#unpack with format 'l' with modifier '_'" do
      it_behaves_like :string_unpack_32bit_be, 'l_'
      it_behaves_like :string_unpack_32bit_be_signed, 'l'
    end

    describe "String#unpack with format 'l' with modifier '!'" do
      it_behaves_like :string_unpack_32bit_be, 'l!'
      it_behaves_like :string_unpack_32bit_be_signed, 'l'
    end
  end

  platform_is :wordsize => 64 do
    platform_is_not :os => :windows do
      describe "String#unpack with format 'L' with modifier '_'" do
        it_behaves_like :string_unpack_64bit_be, 'L_'
        it_behaves_like :string_unpack_64bit_be_unsigned, 'L_'
      end

      describe "String#unpack with format 'L' with modifier '!'" do
        it_behaves_like :string_unpack_64bit_be, 'L!'
        it_behaves_like :string_unpack_64bit_be_unsigned, 'L!'
      end

      describe "String#unpack with format 'l' with modifier '_'" do
        it_behaves_like :string_unpack_64bit_be, 'l_'
        it_behaves_like :string_unpack_64bit_be_signed, 'l_'
      end

      describe "String#unpack with format 'l' with modifier '!'" do
        it_behaves_like :string_unpack_64bit_be, 'l!'
        it_behaves_like :string_unpack_64bit_be_signed, 'l!'
      end
    end

    platform_is :os => :windows do
      not_compliant_on :jruby do
        describe "String#unpack with format 'L' with modifier '_'" do
          it_behaves_like :string_unpack_32bit_be, 'L_'
          it_behaves_like :string_unpack_32bit_be_unsigned, 'L_'
        end

        describe "String#unpack with format 'L' with modifier '!'" do
          it_behaves_like :string_unpack_32bit_be, 'L!'
          it_behaves_like :string_unpack_32bit_be_unsigned, 'L!'
        end

        describe "String#unpack with format 'l' with modifier '_'" do
          it_behaves_like :string_unpack_32bit_be, 'l_'
          it_behaves_like :string_unpack_32bit_be_signed, 'l_'
        end

        describe "String#unpack with format 'l' with modifier '!'" do
          it_behaves_like :string_unpack_32bit_be, 'l!'
          it_behaves_like :string_unpack_32bit_be_signed, 'l!'
        end
      end

      deviates_on :jruby do
        describe "String#unpack with format 'L' with modifier '_'" do
          it_behaves_like :string_unpack_64bit_be, 'L_'
          it_behaves_like :string_unpack_64bit_be_unsigned, 'L_'
        end

        describe "String#unpack with format 'L' with modifier '!'" do
          it_behaves_like :string_unpack_64bit_be, 'L!'
          it_behaves_like :string_unpack_64bit_be_unsigned, 'L!'
        end

        describe "String#unpack with format 'l' with modifier '_'" do
          it_behaves_like :string_unpack_64bit_be, 'l_'
          it_behaves_like :string_unpack_64bit_be_signed, 'l_'
        end

        describe "String#unpack with format 'l' with modifier '!'" do
          it_behaves_like :string_unpack_64bit_be, 'l!'
          it_behaves_like :string_unpack_64bit_be_signed, 'l!'
        end
      end
    end
  end
end
