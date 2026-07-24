use yuzu_anf::AnfCtx;
use yuzu_ast::ast::{AstNode, Root};
use yuzu_core::adt::StringInterner;
use yuzu_diagnostics::{
    diagnostics::{Severity, engine::DiagnosticsEngine, printer::DiagnosticPrinter},
    source_map::SourceMap,
};
use yuzu_hir::HirCtx;
use yuzu_lexer::lexer::{Lexer, Token};
use yuzu_types::TypeCtx;

#[derive(Default)]
pub struct CompileOptions {
    pub debug_tokens: bool,
    pub debug_ast: bool,
    pub debug_hir: bool,
    pub debug_anf: bool,
    pub debug_reduce: bool,
    pub debug_substrait: bool,
}

pub fn compile(name: &str, source: &str, options: &CompileOptions) {
    let mut diagnostics = DiagnosticsEngine::new();
    let mut sources = SourceMap::new();
    let source_id = sources.add(name.to_string(), source.to_string());

    let tokens: Vec<Token> = Lexer::new(source).collect();
    if options.debug_tokens {
        println!("=== tokens ===");
        for token in &tokens {
            println!("{:?}@{:?}", token.kind, token.range);
        }
    }

    let syntax = yuzu_parser::parse(&tokens, &mut diagnostics, source_id);
    if options.debug_ast {
        println!("=== syntax ===\n{syntax:#?}");
    }

    let Some(root) = Root::cast(syntax) else {
        print_diagnostics(&diagnostics, &sources);
        return;
    };

    let mut hir = HirCtx::new();
    let mut interner = StringInterner::new();
    let (root, source_map) =
        yuzu_hir::lower(root, &mut hir, &mut interner, &mut diagnostics, source_id);
    if options.debug_hir {
        println!("=== hir ===");
        print!("{}", yuzu_hir::dump(&hir, &interner, &root));
    }

    let mut types = TypeCtx::new();
    let inference = yuzu_hir::infer(
        &root,
        &hir,
        &mut interner,
        &mut types,
        &mut diagnostics,
        &source_map,
        source_id,
    );

    let backend = options.debug_anf || options.debug_reduce || options.debug_substrait;
    if backend && !has_errors(&diagnostics) {
        let mut anf = AnfCtx::new();
        let (anf_root, anf_source_map) = yuzu_anf::lower(
            &root,
            &hir,
            &inference,
            &types,
            &mut anf,
            &mut interner,
            &mut diagnostics,
            &source_map,
            source_id,
        );
        if options.debug_anf {
            println!("=== anf ===");
            print!("{}", yuzu_anf::dump(&anf, &interner, &anf_root));
        }
        if options.debug_reduce || options.debug_substrait {
            let (reduced, reduced_source_map) =
                yuzu_anf::reduce(&anf_root, &mut anf, &mut interner, &anf_source_map);
            if options.debug_reduce {
                println!("=== reduced ===");
                print!("{}", yuzu_anf::dump(&anf, &interner, &reduced));
            }
            if options.debug_substrait {
                let plan = yuzu_substrait::emit(
                    &reduced,
                    &anf,
                    &types,
                    &interner,
                    &reduced_source_map,
                    &mut diagnostics,
                    source_id,
                );
                if let Some(plan) = plan {
                    println!("=== substrait ===");
                    println!("{}", yuzu_substrait::to_json(&plan));
                }
            }
        }
    }

    print_diagnostics(&diagnostics, &sources);
}

pub fn compile_to_substrait(
    name: &str,
    source: &str,
    options: &CompileOptions,
) -> Result<Vec<u8>, String> {
    let mut diagnostics = DiagnosticsEngine::new();
    let mut sources = SourceMap::new();
    let source_id = sources.add(name.to_string(), source.to_string());

    let tokens: Vec<Token> = Lexer::new(source).collect();
    if options.debug_tokens {
        println!("=== tokens ===");
        for token in &tokens {
            println!("{:?}@{:?}", token.kind, token.range);
        }
    }

    let syntax = yuzu_parser::parse(&tokens, &mut diagnostics, source_id);
    if options.debug_ast {
        println!("=== syntax ===\n{syntax:#?}");
    }

    let Some(root) = Root::cast(syntax) else {
        return Err(render_diagnostics(&diagnostics, &sources));
    };

    let mut hir = HirCtx::new();
    let mut interner = StringInterner::new();
    let (root, source_map) =
        yuzu_hir::lower(root, &mut hir, &mut interner, &mut diagnostics, source_id);
    if options.debug_hir {
        println!("=== hir ===");
        print!("{}", yuzu_hir::dump(&hir, &interner, &root));
    }

    let mut types = TypeCtx::new();
    let inference = yuzu_hir::infer(
        &root,
        &hir,
        &mut interner,
        &mut types,
        &mut diagnostics,
        &source_map,
        source_id,
    );
    if has_errors(&diagnostics) {
        return Err(render_diagnostics(&diagnostics, &sources));
    }

    let mut anf = AnfCtx::new();
    let (anf_root, anf_source_map) = yuzu_anf::lower(
        &root,
        &hir,
        &inference,
        &types,
        &mut anf,
        &mut interner,
        &mut diagnostics,
        &source_map,
        source_id,
    );
    if options.debug_anf {
        println!("=== anf ===");
        print!("{}", yuzu_anf::dump(&anf, &interner, &anf_root));
    }

    let (reduced, reduced_source_map) =
        yuzu_anf::reduce(&anf_root, &mut anf, &mut interner, &anf_source_map);
    if options.debug_reduce {
        println!("=== reduced ===");
        print!("{}", yuzu_anf::dump(&anf, &interner, &reduced));
    }

    let plan = yuzu_substrait::emit(
        &reduced,
        &anf,
        &types,
        &interner,
        &reduced_source_map,
        &mut diagnostics,
        source_id,
    );
    if has_errors(&diagnostics) {
        return Err(render_diagnostics(&diagnostics, &sources));
    }
    let Some(plan) = plan else {
        return Err("the program has no query".to_string());
    };
    if options.debug_substrait {
        println!("=== substrait ===");
        println!("{}", yuzu_substrait::to_json(&plan));
    }

    Ok(yuzu_substrait::to_protobuf(&plan))
}

fn render_diagnostics(diagnostics: &DiagnosticsEngine, sources: &SourceMap) -> String {
    let printer = DiagnosticPrinter::new(sources);
    diagnostics
        .diagnostics()
        .iter()
        .map(|diagnostic| printer.print(diagnostic))
        .collect::<Vec<_>>()
        .join("\n")
}

fn has_errors(diagnostics: &DiagnosticsEngine) -> bool {
    diagnostics
        .diagnostics()
        .iter()
        .any(|diagnostic| matches!(diagnostic.severity, Severity::Error))
}

fn print_diagnostics(diagnostics: &DiagnosticsEngine, sources: &SourceMap) {
    let printer = DiagnosticPrinter::new(sources);
    for diagnostic in diagnostics.diagnostics() {
        eprintln!("{}", printer.print(diagnostic));
    }
}
