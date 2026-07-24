use proc_macro::TokenStream;
use quote::{format_ident, quote};
use syn::{Data, DeriveInput, Fields, parse_macro_input};

/// Derives `TreeCopy` for an ANF node: the node rebuilds itself by calling
/// `copy_tree` on every field, uniformly. The macro decides nothing — each
/// field's own `TreeCopy` impl routes tree ids through the copier and passes
/// pool ids and plain data through unchanged.
///
/// Internal to `yuzu_anf`: the generated code names the traits at
/// `crate::anf::{TreeCopy, TreeCopier}`.
#[proc_macro_derive(TreeCopy)]
pub fn derive_tree_copy(input: TokenStream) -> TokenStream {
    let input = parse_macro_input!(input as DeriveInput);
    let name = &input.ident;

    let body = match &input.data {
        Data::Struct(data) => {
            let fields = copy_fields(&data.fields, quote! { Self });
            quote! { match self { #fields } }
        }
        Data::Enum(data) => {
            let arms = data.variants.iter().map(|variant| {
                let ident = &variant.ident;
                copy_fields(&variant.fields, quote! { Self::#ident })
            });
            quote! { match self { #(#arms)* } }
        }
        Data::Union(_) => {
            return syn::Error::new_spanned(&input.ident, "TreeCopy cannot be derived for unions")
                .to_compile_error()
                .into();
        }
    };

    quote! {
        impl crate::anf::TreeCopy for #name {
            fn copy_tree(&self, copier: &mut impl crate::anf::TreeCopier) -> Self {
                #body
            }
        }
    }
    .into()
}

/// One match arm: destructure the fields and rebuild them through `copy_tree`.
fn copy_fields(fields: &Fields, path: proc_macro2::TokenStream) -> proc_macro2::TokenStream {
    match fields {
        Fields::Named(fields) => {
            let names: Vec<_> = fields
                .named
                .iter()
                .map(|field| field.ident.clone().expect("named field has a name"))
                .collect();
            quote! {
                #path { #(#names),* } => #path {
                    #(#names: crate::anf::TreeCopy::copy_tree(#names, copier)),*
                },
            }
        }
        Fields::Unnamed(fields) => {
            let names: Vec<_> = (0..fields.unnamed.len())
                .map(|index| format_ident!("field{index}"))
                .collect();
            quote! {
                #path(#(#names),*) => #path(
                    #(crate::anf::TreeCopy::copy_tree(#names, copier)),*
                ),
            }
        }
        Fields::Unit => quote! { #path => #path, },
    }
}
