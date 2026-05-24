{
  description = "GoodNet protocol layer: gnet-v1 mesh-framing implementation.";

  inputs = {
    nixpkgs.url     = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    goodnet.url     = "github:GoodNet-io/goodnet/dev";
  };

  outputs = { self, nixpkgs, flake-utils, goodnet }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs   = import nixpkgs { inherit system; };
        kernel = goodnet.packages.${system}.goodnet-core or goodnet.packages.${system}.default;
      in {
        packages.default = pkgs.stdenv.mkDerivation {
          pname   = "goodnet-protocol-gnet";
          version = "1.0.0-rc6";
          src     = ./.;
          nativeBuildInputs = [ pkgs.cmake pkgs.ninja pkgs.pkg-config ];
          buildInputs       = [ kernel pkgs.libsodium ];
          cmakeFlags        = [ "-DCMAKE_BUILD_TYPE=Release" "-DBUILD_TESTING=OFF" ];
          meta = {
            description = "GoodNet gnet-v1 mesh-framing protocol layer.";
            license     = pkgs.lib.licenses.gpl2Only;
          };
        };

        devShells.default = pkgs.mkShell {
          packages = [ kernel pkgs.libsodium pkgs.cmake pkgs.ninja pkgs.pkg-config ];
        };
      });
}
